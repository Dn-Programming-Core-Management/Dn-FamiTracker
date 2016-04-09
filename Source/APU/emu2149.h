/* emu2149.h */
#ifndef _EMU2149_H_
#define _EMU2149_H_

#ifdef EMU2149_DLL_EXPORTS
#define EMU2149_API __declspec(dllexport)
#elif  EMU2149_DLL_IMPORTS
#define EMU2149_API __declspec(dllimport)
#else
#define EMU2149_API
#endif

#define EMU2149_VOL_DEFAULT 1
#define EMU2149_VOL_YM2149 0
#define EMU2149_VOL_AY_3_8910 1

#define PSG_MASK_CH(x) (1<<(x))

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct __PSG
  {

    /* Volume Table */
    uint32_t *voltbl;

    uint8_t reg[0x20];
    int32_t out;
    int32_t cout[3];

    uint32_t clk, rate, base_incr, quality;

    uint32_t count[3];
    uint32_t volume[3];
    uint32_t freq[3];
    uint32_t edge[3];
    uint32_t tmask[3];
    uint32_t nmask[3];
    uint32_t mask;

    uint32_t base_count;

    uint32_t env_volume;
    uint32_t env_ptr;
    uint32_t env_face;

    uint32_t env_continue;
    uint32_t env_attack;
    uint32_t env_alternate;
    uint32_t env_hold;
    uint32_t env_pause;
    uint32_t env_reset;

    uint32_t env_freq;
    uint32_t env_count;

    uint32_t noise_seed;
    uint32_t noise_count;
    uint32_t noise_freq;

    /* rate converter */
    uint32_t realstep;
    uint32_t psgtime;
    uint32_t psgstep;

    /* I/O Ctrl */
    uint32_t adr;

  }
  PSG;

  EMU2149_API void PSG_set_quality (PSG * psg, uint32_t q);
  EMU2149_API void PSG_set_rate (PSG * psg, uint32_t r);
  EMU2149_API PSG *PSG_new (uint32_t clk, uint32_t rate);
  EMU2149_API void PSG_reset (PSG *);
  EMU2149_API void PSG_delete (PSG *);
  EMU2149_API void PSG_writeReg (PSG *, uint32_t reg, uint32_t val);
  EMU2149_API void PSG_writeIO (PSG * psg, uint32_t adr, uint32_t val);
  EMU2149_API uint8_t PSG_readReg (PSG * psg, uint32_t reg);
  EMU2149_API uint8_t PSG_readIO (PSG * psg);
  EMU2149_API int16_t PSG_calc (PSG *);
  EMU2149_API void PSG_setVolumeMode (PSG * psg, int type);
  EMU2149_API uint32_t PSG_setMask (PSG *, uint32_t mask);
  EMU2149_API uint32_t PSG_toggleMask (PSG *, uint32_t mask);

  int32_t PSG_getchanvol(int i);

#ifdef __cplusplus
}
#endif

#endif
