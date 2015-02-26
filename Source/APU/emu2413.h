#ifndef _EMU2413_H_
#define _EMU2413_H_

#ifndef _MAIN_H_
typedef unsigned int uint32 ;
typedef int	int32 ;
typedef signed short int16 ;
typedef unsigned short uint16 ;
typedef signed char int8 ;
typedef unsigned char uint8 ;
#endif

#ifdef EMU2413_DLL_EXPORTS
  #define EMU2413_API __declspec(dllexport)
#elif defined(EMU2413_DLL_IMPORTS)
  #define EMU2413_API __declspec(dllimport)
#else
  #define EMU2413_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PI 3.14159265358979323846

enum OPLL_TONE_ENUM {OPLL_2413_TONE=0, OPLL_VRC7_TONE=1, OPLL_281B_TONE=2} ;

/* voice data */
typedef struct __OPLL_PATCH {
  uint32 TL,FB,EG,ML,AR,DR,SL,RR,KR,KL,AM,PM,WF ;
} OPLL_PATCH ;

/* slot */
typedef struct __OPLL_SLOT {

  OPLL_PATCH *patch;  

  int32 type ;          /* 0 : modulator 1 : carrier */

  /* OUTPUT */
  int32 feedback ;
  int32 output[2] ;   /* Output value of slot */

  /* for Phase Generator (PG) */
  uint16 *sintbl ;    /* Wavetable */
  uint32 phase ;      /* Phase */
  uint32 dphase ;     /* Phase increment amount */
  uint32 pgout ;      /* output */

  /* for Envelope Generator (EG) */
  int32 fnum ;          /* F-Number */
  int32 block ;         /* Block */
  int32 volume ;        /* Current volume */
  int32 sustine ;       /* Sustine 1 = ON, 0 = OFF */
  uint32 tll ;	      /* Total Level + Key scale level*/
  uint32 rks ;        /* Key scale offset (Rks) */
  int32 eg_mode ;       /* Current state */
  uint32 eg_phase ;   /* Phase */
  uint32 eg_dphase ;  /* Phase increment amount */
  uint32 egout ;      /* output */

} OPLL_SLOT ;

/* Mask */
#define OPLL_MASK_CH(x) (1<<(x))
#define OPLL_MASK_HH (1<<(9))
#define OPLL_MASK_CYM (1<<(10))
#define OPLL_MASK_TOM (1<<(11))
#define OPLL_MASK_SD (1<<(12))
#define OPLL_MASK_BD (1<<(13))
#define OPLL_MASK_RHYTHM ( OPLL_MASK_HH | OPLL_MASK_CYM | OPLL_MASK_TOM | OPLL_MASK_SD | OPLL_MASK_BD )

/* opll */
typedef struct __OPLL {

  uint32 adr ;
  int32 out ;

#ifndef EMU2413_COMPACTION
  uint32 realstep ;
  uint32 oplltime ;
  uint32 opllstep ;
  int32 prev, next ;
  int32 sprev[2],snext[2];
  uint32 pan[16];
#endif

  /* Register */
  uint8 reg[0x40] ; 
  int32 slot_on_flag[18] ;

  /* Pitch Modulator */
  uint32 pm_phase ;
  int32 lfo_pm ;

  /* Amp Modulator */
  int32 am_phase ;
  int32 lfo_am ;

  uint32 quality;

  /* Noise Generator */
  uint32 noise_seed ;

  /* Channel Data */
  int32 patch_number[9];
  int32 key_status[9] ;

  /* Slot */
  OPLL_SLOT slot[18] ;

  /* Voice Data */
  OPLL_PATCH patch[19*2] ;
  int32 patch_update[2] ; /* flag for check patch update */

  uint32 mask ;

} OPLL ;

/* Create Object */
EMU2413_API OPLL *OPLL_new(uint32 clk, uint32 rate) ;
EMU2413_API void OPLL_delete(OPLL *) ;

/* Setup */
EMU2413_API void OPLL_reset(OPLL *) ;
EMU2413_API void OPLL_reset_patch(OPLL *, int32) ;
EMU2413_API void OPLL_set_rate(OPLL *opll, uint32 r) ;
EMU2413_API void OPLL_set_quality(OPLL *opll, uint32 q) ;
EMU2413_API void OPLL_set_pan(OPLL *, uint32 ch, uint32 pan);

/* Port/Register access */
EMU2413_API void OPLL_writeIO(OPLL *, uint32 reg, uint32 val) ;
EMU2413_API void OPLL_writeReg(OPLL *, uint32 reg, uint32 val) ;

/* Synthsize */
EMU2413_API int16 OPLL_calc(OPLL *) ;
EMU2413_API void OPLL_calc_stereo(OPLL *, int32 out[2]) ;

/* Misc */
EMU2413_API void OPLL_setPatch(OPLL *, const uint8 *dump) ;
EMU2413_API void OPLL_copyPatch(OPLL *, int32, OPLL_PATCH *) ;
EMU2413_API void OPLL_forceRefresh(OPLL *) ;
/* Utility */
EMU2413_API void OPLL_dump2patch(const uint8 *dump, OPLL_PATCH *patch) ;
EMU2413_API void OPLL_patch2dump(const OPLL_PATCH *patch, uint8 *dump) ;
EMU2413_API void OPLL_getDefaultPatch(int32 type, int32 num, OPLL_PATCH *) ;

/* Channel Mask */
EMU2413_API uint32 OPLL_setMask(OPLL *, uint32 mask) ;
EMU2413_API uint32 OPLL_toggleMask(OPLL *, uint32 mask) ;

#define dump2patch OPLL_dump2patch

int32 OPLL_getchanvol(int i);

#ifdef __cplusplus
}
#endif

#endif

