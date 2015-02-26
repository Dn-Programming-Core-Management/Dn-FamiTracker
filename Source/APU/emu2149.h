/* emu2149.h */
#ifndef _EMU2149_H_
#define _EMU2149_H_

#ifndef _MAIN_H_
typedef unsigned int uint32 ;
typedef int	int32 ;
typedef signed short int16 ;
typedef unsigned short uint16 ;
typedef signed char int8 ;
typedef unsigned char uint8 ;
#endif

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
    uint32 *voltbl;

    uint8 reg[0x20];
    int32 out;
    int32 cout[3];

    uint32 clk, rate, base_incr, quality;

    uint32 count[3];
    uint32 volume[3];
    uint32 freq[3];
    uint32 edge[3];
    uint32 tmask[3];
    uint32 nmask[3];
    uint32 mask;

    uint32 base_count;

    uint32 env_volume;
    uint32 env_ptr;
    uint32 env_face;

    uint32 env_continue;
    uint32 env_attack;
    uint32 env_alternate;
    uint32 env_hold;
    uint32 env_pause;
    uint32 env_reset;

    uint32 env_freq;
    uint32 env_count;

    uint32 noise_seed;
    uint32 noise_count;
    uint32 noise_freq;

    /* rate converter */
    uint32 realstep;
    uint32 psgtime;
    uint32 psgstep;

    /* I/O Ctrl */
    uint32 adr;

  }
  PSG;

  EMU2149_API void PSG_set_quality (PSG * psg, uint32 q);
  EMU2149_API void PSG_set_rate (PSG * psg, uint32 r);
  EMU2149_API PSG *PSG_new (uint32 clk, uint32 rate);
  EMU2149_API void PSG_reset (PSG *);
  EMU2149_API void PSG_delete (PSG *);
  EMU2149_API void PSG_writeReg (PSG *, uint32 reg, uint32 val);
  EMU2149_API void PSG_writeIO (PSG * psg, uint32 adr, uint32 val);
  EMU2149_API uint8 PSG_readReg (PSG * psg, uint32 reg);
  EMU2149_API uint8 PSG_readIO (PSG * psg);
  EMU2149_API int16 PSG_calc (PSG *);
  EMU2149_API void PSG_setVolumeMode (PSG * psg, int type);
  EMU2149_API uint32 PSG_setMask (PSG *, uint32 mask);
  EMU2149_API uint32 PSG_toggleMask (PSG *, uint32 mask);

  int32 PSG_getchanvol(int i);

#ifdef __cplusplus
}
#endif

#endif
