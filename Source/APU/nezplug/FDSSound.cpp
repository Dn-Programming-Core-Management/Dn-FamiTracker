#include <cmath>
#include <memory>
#include <cstdint>		// // //
#include "../APU.h"

// Code is from nezplug via nintendulator

#define LOG_BITS 12
#define LIN_BITS 7
#define LOG_LIN_BITS 30

uint32_t LinearToLog(int32_t l);
int32_t LogToLinear(uint32_t l, uint32_t sft);
void LogTableInitialize(void);

static uint32_t lineartbl[(1 << LIN_BITS) + 1];
static uint32_t logtbl[1 << LOG_BITS];

uint32_t LinearToLog(int32_t l)
{
	return (l < 0) ? (lineartbl[-l] + 1) : lineartbl[l];
}

int32_t LogToLinear(uint32_t l, uint32_t sft)
{
	int32_t ret;
	uint32_t ofs;
	l += sft << (LOG_BITS + 1);
	sft = l >> (LOG_BITS + 1);
	if (sft >= LOG_LIN_BITS) return 0;
	ofs = (l >> 1) & ((1 << LOG_BITS) - 1);
	ret = logtbl[ofs] >> sft;
	return (l & 1) ? -ret : ret;
}

void LogTableInitialize(void)
{
	static volatile uint32_t initialized = 0;
	uint32_t i;
	double a;
	if (initialized) return;
	initialized = 1;
	for (i = 0; i < (1 << LOG_BITS); i++)
	{
		a = (1 << LOG_LIN_BITS) / pow(2, i / (double)(1 << LOG_BITS));
		logtbl[i] = (uint32_t)a;
	}
	lineartbl[0] = LOG_LIN_BITS << LOG_BITS;
	for (i = 1; i < (1 << LIN_BITS) + 1; i++)
	{
		uint32_t ua;
		a = i << (LOG_LIN_BITS - LIN_BITS);
		ua = (uint32_t)((LOG_LIN_BITS - (double(log(a)) / double(log(2.0)))) * (1 << LOG_BITS));
		lineartbl[i] = ua << 1;
	}
}


void FDSSoundInstall(void);
void FDSSelect(unsigned type);


#define FM_DEPTH 0 /* 0,1,2 */
#define NES_BASECYCLES (21477270)
#define PGCPS_BITS (32-16-6)
#define EGCPS_BITS (12)
#define VOL_BITS 12

typedef struct {
	uint8_t spd;
	uint8_t cnt;
	uint8_t mode;
	uint8_t volume;
} FDS_EG;
typedef struct {
	uint32_t spdbase;
	uint32_t spd;
	uint32_t freq;
} FDS_PG;
typedef struct {
	uint32_t phase;
	int8_t wave[0x40];
	uint8_t wavptr;
	int8_t output;
	uint8_t disable;
	uint8_t disable2;
} FDS_WG;
typedef struct {
	FDS_EG eg;
	FDS_PG pg;
	FDS_WG wg;
	int32_t bias;
	uint8_t wavebase;
	uint8_t d[2];
} FDS_OP;

typedef struct FDSSOUND_tag {
	FDS_OP op[2];
	uint32_t phasecps;
	uint32_t envcnt;
	uint32_t envspd;
	uint32_t envcps;
	uint8_t envdisable;
	uint8_t d[3];
	uint32_t lvl;
	int32_t mastervolumel[4];
	uint32_t mastervolume;
	uint32_t srate;
	uint8_t reg[0x10];
} FDSSOUND;

static FDSSOUND fdssound;

static void FDSSoundWGStep(FDS_WG *pwg)
{
#if 0
	if (pwg->disable | pwg->disable2)
		pwg->output = 0;
	else
		pwg->output = pwg->wave[(pwg->phase >> (PGCPS_BITS+16)) & 0x3f];
#else
	if (pwg->disable || pwg->disable2) return;
	pwg->output = pwg->wave[(pwg->phase >> (PGCPS_BITS+16)) & 0x3f];
#endif
}

static void FDSSoundEGStep(FDS_EG *peg)
{
	if (peg->mode & 0x80) return;
	if (++peg->cnt <= peg->spd) return;
	peg->cnt = 0;
	if (peg->mode & 0x40)
		peg->volume += (peg->volume < 0x1f);
	else
		peg->volume -= (peg->volume > 0);
}


int32_t __fastcall FDSSoundRender(void)
{
	int32_t output;
	/* Wave Generator */
	FDSSoundWGStep(&fdssound.op[0].wg);
	// EDIT not using FDSSoundWGStep for modulator (op[1]), need to adjust bias when sample changes

	/* Frequency Modulator */
	fdssound.op[1].pg.spd = fdssound.op[1].pg.spdbase;
	if (fdssound.op[1].wg.disable)
		fdssound.op[0].pg.spd = fdssound.op[0].pg.spdbase;
	else
	{
		// EDIT this step has been entirely rewritten to match FDS.txt by Disch

		// advance the mod table wave and adjust the bias when/if next table entry is reached
		const uint32_t ENTRY_WIDTH = 1 << (PGCPS_BITS + 16);
		uint32_t spd = fdssound.op[1].pg.spd; // phase to add
		while (spd)
		{
			uint32_t left = ENTRY_WIDTH - (fdssound.op[1].wg.phase & (ENTRY_WIDTH-1));
			uint32_t advance = spd;
			if (spd >= left) // advancing to the next entry
			{
				advance = left;
				fdssound.op[1].wg.phase += advance;
				fdssound.op[1].wg.output = fdssound.op[1].wg.wave[(fdssound.op[1].wg.phase >> (PGCPS_BITS+16)) & 0x3f];

				// adjust bias
				int8_t value = fdssound.op[1].wg.output & 7;
				const int8_t MOD_ADJUST[8] = { 0, 1, 2, 4, 0, -4, -2, -1 };
				if (value == 4)
					fdssound.op[1].bias = 0;
				else
					fdssound.op[1].bias += MOD_ADJUST[value];
				while (fdssound.op[1].bias >  63) fdssound.op[1].bias -= 128;
				while (fdssound.op[1].bias < -64) fdssound.op[1].bias += 128;
			}
			else // not advancing to the next entry
			{
				fdssound.op[1].wg.phase += advance;
			}
			spd -= advance;
		}

		// modulation calculation
		int32_t mod = fdssound.op[1].bias * (int32_t)(fdssound.op[1].eg.volume);
		mod >>= 4;
		if (mod & 0x0F)
		{
			if (fdssound.op[1].bias < 0) mod -= 1;
			else                         mod += 2;
		}
		if (mod > 193) mod -= 258;
		if (mod < -64) mod += 256;
		mod = (mod * (int32_t)(fdssound.op[0].pg.freq)) >> 6;

		// calculate new frequency with modulation
		int32_t new_freq = fdssound.op[0].pg.freq + mod;
		if (new_freq < 0) new_freq = 0;
		fdssound.op[0].pg.spd = (uint32_t)(new_freq) * fdssound.phasecps;
	}

	/* Accumulator */
	output = fdssound.op[0].eg.volume;
	if (output > 0x20) output = 0x20;
	output = (fdssound.op[0].wg.output * output * fdssound.mastervolumel[fdssound.lvl]) >> (VOL_BITS - 4);

	/* Envelope Generator */
	if (!fdssound.envdisable && fdssound.envspd)
	{
		fdssound.envcnt += fdssound.envcps;
		while (fdssound.envcnt >= fdssound.envspd)
		{
			fdssound.envcnt -= fdssound.envspd;
			FDSSoundEGStep(&fdssound.op[1].eg);
			FDSSoundEGStep(&fdssound.op[0].eg);
		}
	}

	/* Phase Generator */
	fdssound.op[0].wg.phase += fdssound.op[0].pg.spd;
	// EDIT modulator op[1] phase now updated above.

	return (fdssound.op[0].pg.freq != 0) ? output : 0;
}

void __fastcall FDSSoundVolume(unsigned int volume)
{
	volume += 196;
	fdssound.mastervolume = (volume << (LOG_BITS - 8)) << 1;
	fdssound.mastervolumel[0] = LogToLinear(fdssound.mastervolume, LOG_LIN_BITS - LIN_BITS - VOL_BITS) * 2;
	fdssound.mastervolumel[1] = LogToLinear(fdssound.mastervolume, LOG_LIN_BITS - LIN_BITS - VOL_BITS) * 4 / 3;
	fdssound.mastervolumel[2] = LogToLinear(fdssound.mastervolume, LOG_LIN_BITS - LIN_BITS - VOL_BITS) * 2 / 2;
	fdssound.mastervolumel[3] = LogToLinear(fdssound.mastervolume, LOG_LIN_BITS - LIN_BITS - VOL_BITS) * 8 / 10;
}

static const uint8_t wave_delta_table[8] = {
	0,(1 << FM_DEPTH),(2 << FM_DEPTH),(4 << FM_DEPTH),
	0,256 - (4 << FM_DEPTH),256 - (2 << FM_DEPTH),256 - (1 << FM_DEPTH),
};

void __fastcall FDSSoundWrite(uint16_t address, uint8_t value)
{
	if (0x4040 <= address && address <= 0x407F)
	{
		fdssound.op[0].wg.wave[address - 0x4040] = ((int)(value & 0x3f)) - 0x20;
	}
	else if (0x4080 <= address && address <= 0x408F)
	{
		FDS_OP *pop = &fdssound.op[(address & 4) >> 2];
		fdssound.reg[address - 0x4080] = value;
		switch (address & 0xf)
		{
			case 0:
			case 4:
				pop->eg.mode = value & 0xc0;
				if (pop->eg.mode & 0x80)
				{
					pop->eg.volume = (value & 0x3f);
				}
				else
				{
					pop->eg.spd = value & 0x3f;
				}
				break;
			case 5:
				// EDIT rewrote modulator/bias code
				fdssound.op[1].bias = value & 0x3F;
				if (value & 0x40) fdssound.op[1].bias -= 0x40; // extend sign bit
				fdssound.op[1].wg.phase = 0;
				break;
			case 2:	case 6:
				pop->pg.freq &= 0x00000F00;
				pop->pg.freq |= (value & 0xFF) << 0;
				pop->pg.spdbase = pop->pg.freq * fdssound.phasecps;
				break;
			case 3:
				fdssound.envdisable = value & 0x40;
			case 7:
#if 0
				pop->wg.phase = 0;
#endif
				pop->pg.freq &= 0x000000FF;
				pop->pg.freq |= (value & 0x0F) << 8;
				pop->pg.spdbase = pop->pg.freq * fdssound.phasecps;
				pop->wg.disable = value & 0x80;
				if (pop->wg.disable)
				{
					pop->wg.phase = 0;
					pop->wg.wavptr = 0;
					pop->wavebase = 0;
				}
				break;
			case 8:
				// EDIT rewrote modulator/bias code
				if (fdssound.op[1].wg.disable)
				{
					int8_t append = value & 0x07;
					for (int i=0; i < 0x3E; ++i)
					{
						fdssound.op[1].wg.wave[i] = fdssound.op[1].wg.wave[i+2];
					}
					fdssound.op[1].wg.wave[0x3E] = append;
					fdssound.op[1].wg.wave[0x3F] = append;
				}
				break;
			case 9:
				fdssound.lvl = (value & 3);
				fdssound.op[0].wg.disable2 = value & 0x80;
				break;
			case 10:
				fdssound.envspd = value << EGCPS_BITS;
				break;
			default:
				break;
		}
	}
}

uint8_t __fastcall FDSSoundRead(uint16_t address)
{
	if (0x4040 <= address && address <= 0x407f)
	{
		return fdssound.op[0].wg.wave[address & 0x3f] + 0x20;
	}
	if (0x4090 == address)
		return fdssound.op[0].eg.volume | 0x40;
	if (0x4092 == address) /* 4094? */
		return fdssound.op[1].eg.volume | 0x40;
	return 0;
}

static uint32_t DivFix(uint32_t p1, uint32_t p2, uint32_t fix)
{
	uint32_t ret;
	ret = p1 / p2;
	p1  = p1 % p2;/* p1 = p1 - p2 * ret; */
	while (fix--)
	{
		p1 += p1;
		ret += ret;
		if (p1 >= p2)
		{
			p1 -= p2;
			ret++;
		}
	}
	return ret;
}

void __fastcall FDSSoundReset(void)
{
	uint32_t i;
	memset(&fdssound, 0, sizeof(FDSSOUND));
	// TODO: Fix srate
	fdssound.srate = CAPU::BASE_FREQ_NTSC; ///NESAudioFrequencyGet();
	fdssound.envcps = DivFix(NES_BASECYCLES, 12 * fdssound.srate, EGCPS_BITS + 5 - 9 + 1);
	fdssound.envspd = 0xe8 << EGCPS_BITS;
	fdssound.envdisable = 1;
	fdssound.phasecps = DivFix(NES_BASECYCLES, 12 * fdssound.srate, PGCPS_BITS);
	for (i = 0; i < 0x40; i++)
	{
		fdssound.op[0].wg.wave[i] = (i < 0x20) ? 0x1f : -0x20;
		fdssound.op[1].wg.wave[i] = 64;
	}
}

void FDSSoundInstall3(void)
{
	LogTableInitialize();

}
