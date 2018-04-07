#ifndef _EMU2413_H_
#define _EMU2413_H_

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
  uint32_t TL,FB,EG,ML,AR,DR,SL,RR,KR,KL,AM,PM,WF ;
} OPLL_PATCH ;

/* slot */
typedef struct __OPLL_SLOT {

  OPLL_PATCH *patch;  

  int32_t type ;          /* 0 : modulator 1 : carrier */

  /* OUTPUT */
  int32_t feedback ;
  int32_t output[2] ;   /* Output value of slot */

  /* for Phase Generator (PG) */
  uint16_t *sintbl ;    /* Wavetable */
  uint32_t phase ;      /* Phase */
  uint32_t dphase ;     /* Phase increment amount */
  uint32_t pgout ;      /* output */

  /* for Envelope Generator (EG) */
  int32_t fnum ;          /* F-Number */
  int32_t block ;         /* Block */
  int32_t volume ;        /* Current volume */
  int32_t sustine ;       /* Sustine 1 = ON, 0 = OFF */
  uint32_t tll ;	      /* Total Level + Key scale level*/
  uint32_t rks ;        /* Key scale offset (Rks) */
  int32_t eg_mode ;       /* Current state */
  uint32_t eg_phase ;   /* Phase */
  uint32_t eg_dphase ;  /* Phase increment amount */
  uint32_t egout ;      /* output */

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

  uint32_t adr ;
  int32_t out ;

#ifndef EMU2413_COMPACTION
  uint32_t realstep ;
  uint32_t oplltime ;
  uint32_t opllstep ;
  int32_t prev, next ;
  int32_t sprev[2],snext[2];
  uint32_t pan[16];
#endif

  /* Register */
  uint8_t reg[0x40] ; 
  int32_t slot_on_flag[18] ;

  /* Pitch Modulator */
  uint32_t pm_phase ;
  int32_t lfo_pm ;

  /* Amp Modulator */
  int32_t am_phase ;
  int32_t lfo_am ;

  uint32_t quality;

  /* Noise Generator */
  uint32_t noise_seed ;

  /* Channel Data */
  int32_t patch_number[9];
  int32_t key_status[9] ;

  /* Slot */
  OPLL_SLOT slot[18] ;

  /* Voice Data */
  OPLL_PATCH patch[19*2] ;
  int32_t patch_update[2] ; /* flag for check patch update */

  uint32_t mask ;

} OPLL ;

/* Create Object */
EMU2413_API OPLL *OPLL_new(uint32_t clk, uint32_t rate) ;
EMU2413_API void OPLL_delete(OPLL *) ;

/* Setup */
EMU2413_API void OPLL_reset(OPLL *) ;
EMU2413_API void OPLL_reset_patch(OPLL *, int32_t) ;
EMU2413_API void OPLL_set_rate(OPLL *opll, uint32_t r) ;
EMU2413_API void OPLL_set_quality(OPLL *opll, uint32_t q) ;
EMU2413_API void OPLL_set_pan(OPLL *, uint32_t ch, uint32_t pan);

/* Port/Register access */
EMU2413_API void OPLL_writeIO(OPLL *, uint32_t reg, uint32_t val) ;
EMU2413_API void OPLL_writeReg(OPLL *, uint32_t reg, uint32_t val) ;

/* Synthsize */
EMU2413_API int16_t OPLL_calc(OPLL *) ;
EMU2413_API void OPLL_calc_stereo(OPLL *, int32_t out[2]) ;

/* Misc */
EMU2413_API void OPLL_setPatch(OPLL *, const uint8_t *dump) ;
EMU2413_API void OPLL_copyPatch(OPLL *, int32_t, OPLL_PATCH *) ;
EMU2413_API void OPLL_forceRefresh(OPLL *) ;
/* Utility */
EMU2413_API void OPLL_dump2patch(const uint8_t *dump, OPLL_PATCH *patch) ;
EMU2413_API void OPLL_patch2dump(const OPLL_PATCH *patch, uint8_t *dump) ;
EMU2413_API void OPLL_getDefaultPatch(int32_t type, int32_t num, OPLL_PATCH *) ;

/* Channel Mask */
EMU2413_API uint32_t OPLL_setMask(OPLL *, uint32_t mask) ;
EMU2413_API uint32_t OPLL_toggleMask(OPLL *, uint32_t mask) ;

#define dump2patch OPLL_dump2patch

int32_t OPLL_getchanvol(int i);

#ifdef __cplusplus
}
#endif

#endif

