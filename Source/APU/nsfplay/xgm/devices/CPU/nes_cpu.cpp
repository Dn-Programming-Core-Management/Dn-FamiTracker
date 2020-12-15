#include <assert.h>
#include <cstring>
#include "nes_cpu.h"
#include "../Memory/nes_mem.h"
#include "../Misc/nsf2_irq.h"

#define DEBUG_RW 0
#define TRACE 0

#if TRACE
const char* OP_NAME[256] = {
/*  0x-0   0x-1   0x-2   0x-3   0x-4   0x-5   0x-6   0x-7   0x-8   0x-9   0x-A   0x-B   0x-C   0x-D   0x-E   0x-F           */
    "BRK", "ORA", "kil", "slo", "nop", "ORA", "ASL", "slo", "PHP", "ORA", "ASL", "anc", "nop", "ORA", "ASL", "slo", /* 0x0- */
    "BPL", "ORA", "kil", "slo", "nop", "ORA", "ASL", "slo", "CLC", "ORA", "nop", "slo", "nop", "ORA", "ASL", "slo", /* 0x1- */
    "JSR", "AND", "kil", "rla", "BIT", "AND", "ROL", "rla", "PLP", "AND", "ROL", "anc", "BIT", "AND", "ROL", "rla", /* 0x2- */
    "BMI", "AND", "kil", "rla", "nop", "AND", "ROL", "rla", "SEC", "AND", "nop", "rla", "nop", "AND", "ROL", "rla", /* 0x3- */
    "RTI", "EOR", "kil", "sre", "nop", "EOR", "LSR", "sre", "PHA", "EOR", "LSR", "alr", "JMP", "EOR", "LSR", "sre", /* 0x4- */
    "BVC", "EOR", "kil", "sre", "nop", "EOR", "LSR", "sre", "CLI", "EOR", "nop", "sre", "nop", "EOR", "LSR", "sre", /* 0x5- */
    "RTS", "ADC", "kil", "rra", "nop", "ADC", "ROR", "rra", "PLA", "ADC", "ROR", "arr", "JMP", "ADC", "ROR", "rra", /* 0x6- */
    "BVS", "ADC", "kil", "rra", "nop", "ADC", "ROR", "rra", "SEI", "ADC", "nop", "rra", "nop", "ADC", "ROR", "rra", /* 0x7- */
    "nop", "STA", "nop", "sax", "STY", "STA", "STX", "sax", "DEY", "nop", "TXA", "xaa", "STY", "STA", "STX", "sax", /* 0x8- */
    "BCC", "STA", "kil", "ahx", "STY", "STA", "STX", "sax", "TYA", "STA", "TXS", "tas", "shy", "STA", "shx", "ahx", /* 0x9- */
    "LDY", "LDA", "LDX", "lax", "LDY", "LDA", "LDX", "lax", "TAY", "LDA", "TAX", "lax", "LDY", "LDA", "LDX", "lax", /* 0xA- */
    "BCS", "LDA", "kil", "lax", "LDY", "LDA", "LDX", "lax", "CLV", "LDA", "TSX", "las", "LDY", "LDA", "LDX", "lax", /* 0xB- */
    "CPY", "CMP", "nop", "dcp", "CPY", "CMP", "DEC", "dcp", "INY", "CMP", "DEX", "axs", "CPY", "CMP", "DEC", "dcp", /* 0xC- */
    "BNE", "CMP", "kil", "dcp", "nop", "CMP", "DEC", "dcp", "CLD", "CMP", "nop", "dcp", "nop", "CMP", "DEC", "dcp", /* 0xD- */
    "CPX", "SBC", "nop", "isc", "CPX", "SBC", "INC", "isc", "INX", "SBC", "NOP", "sbc", "CPX", "SBC", "INC", "isc", /* 0xE- */
    "BEQ", "SBC", "kil", "isc", "nop", "SBC", "INC", "isc", "SED", "SBC", "nop", "isc", "nop", "SBC", "INC", "isc"  /* 0xF- */
};
#endif

namespace xgm
{

inline void exec(K6502_Context& context, xgm::IDevice* bus)
{
    #if TRACE
        Uword TPC = context.PC;
        UINT32 tb[3];
        bus->Read((TPC+0) & 0xFFFF, tb[0]);
        bus->Read((TPC+1) & 0xFFFF, tb[1]);
        bus->Read((TPC+2) & 0xFFFF, tb[2]);
        DEBUG_OUT("%04X: A=%02X X=%02X Y=%02X P=%02X S=%02X %c > ",
            context.PC,
            context.A, context.X, context.Y, context.P, context.S,
            context.iRequest ? 'I':'i');
    #endif

    K6502_Exec(&context);

    #if TRACE
        DEBUG_OUT("%s", OP_NAME[context.lastcode]);
        int oplen = context.PC - TPC; // not accurate for branch/jump but good enough
        for (int i=0; i<3; ++i)
        {
            if (i == 0 || i < oplen)
            {
                DEBUG_OUT(" %02X", tb[i]);
            }
        }
        DEBUG_OUT("\n");
    #endif
}

// bits of fixed point for timing
// 16 causes overflow at low update rate values (~27 Hz)
// 15 should be sufficient for any NSF (~13.6 Hz), since the format only allows down to ~15.25 Hz
// 14 used here just for one extra bit of safety
const int FRAME_FIXED = 14;

NES_CPU::NES_CPU (double clock)
{
  nes_basecycles = clock;
  bus = NULL;
  log_cpu = NULL;
  irqs = 0;
  enable_irq = true;
  enable_nmi = false;
  nsf2_bits = 0;
  nsf2_irq = NULL;
}

NES_CPU::~NES_CPU ()
{
}

void Callback writeByte (void *__THIS, UINT32 adr, UINT32 val)
{
  static_cast < NES_CPU * >(__THIS)->Write (adr, val);
}

UINT32 Callback readByte (void *__THIS, UINT32 adr)
{
  UINT32 val = 0;
  static_cast < NES_CPU * >(__THIS)->Read (adr, val);
  return val;
}

void NES_CPU::run_from (UINT32 address)
{
	#if TRACE
		DEBUG_OUT("run_from($%04X)\n",address);
	#endif
	breaked = false;
	context.PC = PLAYER_RESERVED; // JSR, followed by infinite loop ("breaked")
	breakpoint = context.PC+3;
	assert (bus);
	bus->Write (PLAYER_RESERVED+1, address & 0xff);
	bus->Write (PLAYER_RESERVED+2, address>>8);
	// see PLAYER_PROGRAM in nsfplay.cpp
}

int NES_CPU::Exec (int clocks)
{
	// DPCM cycle stealing
	context.clock = stolen_cycles;
	stolen_cycles = 0;

	while ( int(context.clock) < clocks )
	{
		if (breaked)
		{
			if (extra_init)
			{
				enable_nmi = true;
				extra_init = false;
				context.A = song;
				context.X = region;
				context.Y = 0x81;
				run_from(init_addr);
			}

			if (enable_irq && !(context.P & K6502_I_FLAG) && context.iRequest)
			{
				breaked = false;
			}
		}

		Uword clock_start = context.clock;
		if (!breaked)
		{
			//DEBUG_OUT("PC: 0x%04X\n", context.PC);
			exec(context,bus);
			if (context.PC == breakpoint)
			{
				breaked = true;
			}
		}
		else 
		{
			if ( (fclocks_left_in_frame >> FRAME_FIXED) < INT64(clocks) )
			{
				if (fclocks_left_in_frame < 0)
					context.clock = 0;
				else
					context.clock = (unsigned int)(fclocks_left_in_frame >> FRAME_FIXED)+1;
			}
			else
				context.clock = clocks;
		}
		if (nsf2_irq) nsf2_irq->Clock(context.clock-clock_start);

		if (breaked && play_ready)
		{
			if (play_addr >= 0)
			{
				if (log_cpu)
					log_cpu->Play();
				run_from (play_addr);
			}
			play_ready = false;
		}

		// signal next play
		if ( (fclocks_left_in_frame >> FRAME_FIXED) < context.clock)
		{
			if (play_addr >= 0)
			{
				if (nmi_play) // trigger play by NMI
				{
					if (enable_nmi)
					{
						if (log_cpu)
							log_cpu->Play();
						context.iRequest |= IRQ_NMI;
						breaked = false;
					}
					play_ready = false;
				}
				else
				{
					play_ready = true;
				}
			}
			fclocks_left_in_frame += fclocks_per_frame;
			//DEBUG_OUT("NMI\n");
		}
	}

	fclocks_left_in_frame -= (context.clock << FRAME_FIXED);

	return context.clock; // return actual number of clocks executed
}

void NES_CPU::SetMemory (IDevice * b)
{
  bus = b;
}

bool NES_CPU::Write (UINT32 adr, UINT32 val, UINT32 id)
{
  #if DEBUG_RW
    DEBUG_OUT("Write: 0x%04X = 0x%02X\n", adr, val);
  #endif

  // for blargg's CPU tests
  #if 0
    if (adr == 0x6000)
    {
        DEBUG_OUT("Blargg result: %02X [");
        UINT32 msg = 0x6004;
        do
        {
            UINT32 ic;
            Read(msg, ic);
            if (ic == 0) break;
            ++msg;
            DEBUG_OUT("%c", char(ic));
        } while (1);
        DEBUG_OUT("]\n");
        return false;
    }
  #endif

  if (bus)
    return bus->Write (adr, val, id);
  else
    return false;
}

bool NES_CPU::Read (UINT32 adr, UINT32 & val, UINT32 id)
{
  if (bus)
  {
    bool result = bus->Read (adr, val, id);

    #if DEBUG_RW
      DEBUG_OUT(" Read: 0x%04X = 0x%02X\n", adr, val);
    #endif

    return result;
  }
  else
    return false;
}

void NES_CPU::Reset ()
{
  // KM6502‚ÌƒŠƒZƒbƒg
  memset (&context, 0, sizeof (K6502_Context));
  context.ReadByte = readByte;
  context.WriteByte = writeByte;
  context.iRequest = K6502_INIT;
  context.clock = 0;
  context.user = this;
  context.A = 0;
  context.X = 0;
  context.Y = 0;
  context.S = 0xff;
  context.PC = breakpoint = 0xffff;
  context.illegal = 0;
  breaked = false;
  irqs = 0;
  stolen_cycles = 0;
  play_ready = false;
  exec(context, bus);
}

void NES_CPU::Start (
	int init_addr_,
	int play_addr_,
	double play_rate,
	int song_,
	int region_,
	UINT8 nsf2_bits_,
	bool enable_irq_,
	NSF2_IRQ* nsf2_irq_)
{
	// approximate frame timing as an integer number of CPU clocks
	init_addr = init_addr_;
	play_addr = play_addr_;
	song = song_;
	region = region_;
	fclocks_per_frame = (INT64)((double)((1 << FRAME_FIXED) * nes_basecycles) / play_rate );
	fclocks_left_in_frame = 0;
	stolen_cycles = 0;
	play_ready = false;
	irqs = 0;
	nsf2_bits = nsf2_bits_;
	nsf2_irq = nsf2_irq_;
	enable_irq = enable_irq_;
	enable_nmi = false;

	// enable NSF2 IRQ
	if (nsf2_bits & 0x10) enable_irq = true;
	else nsf2_irq = NULL;

	// NSF2 disable PLAY
	if (nsf2_bits & 0x40)
	{
		play_addr = -1;
	}

	extra_init = false;
	nmi_play = false;
	if (nsf2_bits & 0x20)
	{
		extra_init = true;
		nmi_play = true;
	}

	if (log_cpu)
		log_cpu->Init(song, region);

	context.A = song;
	context.X = region;
	context.Y = extra_init ? 0x80 : 0;
	context.P = 0x26; // VIZ

	run_from (init_addr);

	// temporarily disable PLAY and IRQ for INIT
	bool irq_temp = enable_irq;
	bool extra_init_temp = extra_init;
	int play_addr_temp = play_addr;
	enable_irq = false;
	extra_init = false;
	play_addr = -1;

	// run up to 1 second of init before starting real playback (this allows INIT to modify $4011 etc. silently)
	// note: things like DMC, Frame Counter, NSF2 IRQ, MMC Frame Counter, etc. aren't receiving cycles here
	//       but this should be OK?
	//       - Use of IRQs should really be taking place in PLAY or non-returning second INIT.
	//         IRQs are intentionally disabled here during first INIT to avoid issue with IRQs being mistaken
	//         for the audio-free INIT segment, but a hardware implementation probably wouldn't prevent this.
	//       - Timing between end of INIT and first PLAY is not guaranteed by the player and should not be relied on.
	//       - For NSFs that do not reset $4017 this leaves the envelope starting in synch with the first PLAY.
	//       - Waiting on an IRQ during the first init should hit the timeout and eventually trigger.
	//         (Could be a problem if they're trying to count cycles there?)
	int timeout = int(nes_basecycles);
	while (timeout > 0)
	{
		timeout -= Exec(1);
		if (breaked)
		{
			if (nmi_play) enable_nmi = true;
		}
	}
	enable_irq = irq_temp; // restore enable_irq
	extra_init = extra_init_temp; // restore extra_init
	play_addr = play_addr_temp; // restore PLAY

	// start of first frame
	fclocks_left_in_frame = fclocks_per_frame;
	play_ready = breaked && !extra_init;
}

void NES_CPU::SetLogger (CPULogger* logger)
{
	log_cpu = logger;
}

unsigned int NES_CPU::GetPC() const
{
	return context.PC;
}

void NES_CPU::StealCycles(unsigned int cycles)
{
	stolen_cycles += cycles;
}

void NES_CPU::EnableNMI(bool enable)
{
	enable_nmi = enable;
}

void NES_CPU::UpdateIRQ(int device, bool irq)
{
	if (!enable_irq) return;
	if (device < 0 || device >= IRQD_COUNT) return;
	UINT32 mask = 1 << device;
	irqs &= ~mask;
	if (irq) irqs |= mask;
	if (irqs)
	{
		context.iRequest |= IRQ_INT;
	}
	else
	{
		context.iRequest &= ~Uword(IRQ_INT);
	}
}

} // namespace xgm
