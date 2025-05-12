;;; ;; ;
;;; ;; ; Dn-FamiTracker NSF Driver
;;; ;; ; By jsr, HertzDevil, D.P.C.M., etc.
;;; ;; ;

.define DRIVER_NAME "Dn-FT "
; version 2.15
.define VERSION_MAJ 2
.define VERSION_MIN 15

;
; Assembler code switches
;

.if .defined(HAS_NSF_HEADER)
	.segment "HEADER"
	.if .defined(USE_AUX_DATA)
		.include "../nsf_header.s"
	.else
		.include "nsf_wrap.s"
	.endif
.endif

;USE_BANKSWITCH = 1		; Enable bankswitching code
;USE_OLDVIBRATO = 1		;;; ;; ; Enable old vibrato code
;USE_LINEARPITCH = 1		;;; ;; ; Enable linear pitch code

USE_DPCM = 1			; Enable DPCM channel (currently broken, leave enabled to avoid trouble).
						; Also leave enabled when using expansion chips

;INC_MUSIC_ASM = 1		; Music is in assembler style
;RELOCATE_MUSIC = 1		; Enable if music data must be relocated

ENABLE_ROW_SKIP = 1		; Enable this to add code for seeking to a row > 0 when using skip command

;PACKAGE = 1			; Enable this when compiling an .nsf
;USE_VRC6 = 1 			; Enable this to include VRC6 code
;USE_VRC7 = 1			; Enable this to include VRC7 code
;USE_FDS  = 1			; Enable this to include FDS code
;USE_MMC5 = 1			; Enable this to include MMC5 code
;USE_N163 = 1			; Enable this to include N163 code
;USE_S5B  = 1			; Enable this to include 5B code
;USE_ALL  = 1			;;; ;; ; All expansion chips, always assumes 8 N163 channels

;USE_MMC5_MULTIPLIER = 1	;;; ;; ; optimize multiplication using MMC5 hardware multiplier

.if .defined(USE_ALL)	;;; ;; ;
	USE_VRC6 = 1
	USE_VRC7 = 1
	USE_FDS  = 1
	USE_MMC5 = 1
	USE_N163 = 1
	USE_S5B  = 1
.endif

EXPANSION_FLAG = .defined(USE_VRC6) + .defined(USE_VRC7) << 1 + .defined(USE_FDS) << 2 + .defined(USE_MMC5) << 3 + .defined(USE_N163) << 4 + .defined(USE_S5B) << 5

MULTICHIP = (EXPANSION_FLAG & (EXPANSION_FLAG - 1)) <> 0

NTSC_PERIOD_TABLE = 1
.if !(EXPANSION_FLAG)
	PAL_PERIOD_TABLE = 1 ; forbid PAL table when there is any expansion chip
.endif

;ENABLE_SFX = 1			; Enable this to enable sound effect support (not yet working)
;SCALE_NOISE = 1		; Enable 4 bit noise period scaling

;CHANNEL_CONTROL = 1	; Enable to access channel enable/disable routines


;
; Constants
;

; Setup the pattern number -> channel mapping, as exported by the tracker
;;; ;; ; many of these have been renamed for consistency
;;; ;; ; some are moved from the respective chips' asm files

;NAMCO_CHANNELS = 8

CH_COUNT_2A03 = 4
CH_COUNT_MMC5 = 2 * .defined(USE_MMC5)
CH_COUNT_VRC6 = 3 * .defined(USE_VRC6)
CH_COUNT_N163 = NAMCO_CHANNELS * .defined(USE_N163)
CH_COUNT_FDS  = 1 * .defined(USE_FDS)
CH_COUNT_S5B  = 3 * .defined(USE_S5B)
CH_COUNT_VRC7 = 6 * .defined(USE_VRC7)

APU_OFFSET  = 0
MMC5_OFFSET = CH_COUNT_2A03 + APU_OFFSET
VRC6_OFFSET = CH_COUNT_MMC5 + MMC5_OFFSET
N163_OFFSET = CH_COUNT_VRC6 + VRC6_OFFSET
FDS_OFFSET  = CH_COUNT_N163 + N163_OFFSET
S5B_OFFSET  = CH_COUNT_FDS  + FDS_OFFSET
VRC7_OFFSET = CH_COUNT_S5B  + S5B_OFFSET
DPCM_OFFSET = CH_COUNT_VRC7 + VRC7_OFFSET

EFF_CHANS   = DPCM_OFFSET							; # of channels using vibrato & arpeggio effects (not used by DPCM)
WAVE_CHANS  = DPCM_OFFSET - CH_COUNT_VRC7
CHANNELS	= DPCM_OFFSET + .defined(USE_DPCM)

;.if .defined(ENABLE_SFX)
;	SFX_CHANS		= CHANNELS * 2
;	SFX_WAVE_CHANS 	= WAVE_CHANS * 2
;.else
	SFX_CHANS		= CHANNELS
	SFX_WAVE_CHANS 	= WAVE_CHANS
;.endif

.enum
	APU_PU1
	APU_PU2
	APU_TRI		; 0CC: should not be compared against
	APU_NOI		; 0CC: should not be compared against
.endenum
; ;; ;;;

.enum			;;; ;; ; divide by 2
	CHAN_2A03
	CHAN_TRI
	CHAN_NOI
	CHAN_DPCM
	CHAN_VRC6	; used
	CHAN_SAW	; used
	CHAN_VRC7	; used
	CHAN_FDS	; used
	CHAN_MMC5	; used
	CHAN_N163	; used
	CHAN_S5B 	; used
.endenum		; ;; ;;;

.enum
	EFF_NONE
	EFF_ARPEGGIO
	EFF_PORTAMENTO
	EFF_PORTA_UP
	EFF_PORTA_DOWN
	EFF_SLIDE_UP_LOAD
	EFF_SLIDE_UP
	EFF_SLIDE_DOWN_LOAD
	EFF_SLIDE_DOWN
.endenum

.enum
	STATE_RELEASE = %00000001
	STATE_HOLD    = %00000010
.endenum

; must match definitions in CCompiler!
.enum
	FLAG_BANKSWITCH  = 1 << 0
	FLAG_OLDVIBRATO  = 1 << 1
	FLAG_LINEARPITCH = 1 << 2
	FLAG_USEPAL      = 1 << 7
.endenum

.segment "ZEROPAGE"

;
; Variables that must be on zero-page
;
var_Temp:				.res 1						; Temporary 8-bit
var_Temp2:				.res 1
var_Temp3:				.res 1
var_Temp4:				.res 1
var_Temp16:				.res 2						; Temporary 16-bit
var_Temp_Pointer:		.res 2						; Temporary
var_Temp_Pointer2:		.res 2
var_Temp_Pattern:		.res 2						; Pattern address (temporary)
var_Note_Table:			.res 2
var_currentChannel:		.res 1						;;; ;; ;

ACC:					.res 2						; Used by division routine
AUX:					.res 2
EXT:					.res 2

;;; ;; ; all chip-specific variables have been moved to the zeropage

.if .defined(USE_MMC5)
var_ch_LengthCounter:	 .res 6						; LLLLL-HC Length counter, Enable length counter, Enable decay/linear
var_ch_PrevFreqHighMMC5: .res 2
.else
var_ch_LengthCounter:	 .res 4						; MMC5 has two extra pulse channels
.endif
var_Linear_Counter:		 .res 1						; Triangle linear counter

.if .defined(USE_DPCM)
var_ch_SamplePtr:		.res 1						; DPCM sample pointer
var_ch_SampleLen:		.res 1						; DPCM sample length
var_ch_SampleBank:		.res 1						; DPCM sample bank
var_ch_SamplePitch:		.res 1						; DPCM sample pitch
var_ch_DPCMDAC:			.res 1						; DPCM delta counter setting
var_ch_DPCM_Offset:		.res 1
var_ch_DPCM_Retrig:		.res 1						; DPCM retrigger
var_ch_DPCM_RetrigCntr:	.res 1
var_ch_DPCM_EffPitch:	.res 1
.endif

.if .defined(USE_VRC7)
;;; ;; ; removed, since other chips also use 2 bytes for duty
var_ch_vrc7_FnumLo:		 .res CH_COUNT_VRC7
var_ch_vrc7_FnumHi:		 .res CH_COUNT_VRC7
var_ch_vrc7_Bnum:		 .res CH_COUNT_VRC7
var_ch_vrc7_ActiveNote:	 .res CH_COUNT_VRC7
var_ch_vrc7_Command:	 .res CH_COUNT_VRC7			; 0 = halt, 1 = trigger, 80 = update
var_ch_vrc7_OldOctave:	 .res 1						; Temp variable for old octave when triggering new notes
var_ch_vrc7_EffPatch:	 .res CH_COUNT_VRC7			;;; ;; ; V-command

var_ch_vrc7_CustomHi:    .res CH_COUNT_VRC7
var_ch_vrc7_CustomLo:    .res CH_COUNT_VRC7
var_CustomPatchPtr:		 .res 2
var_ch_vrc7_Port:		 .res CH_COUNT_VRC7			;;; ;; ; Hxx
var_ch_vrc7_Write:		 .res 8						;;; ;; ; Ixx
var_ch_vrc7_PatchFlag:	 .res 1
.endif

.if .defined(USE_FDS)
var_ch_ModDelay:		.res 1
var_ch_ModDepth:		.res 1
var_ch_ModRate:			.res 2
var_ch_ModDelayTick:	.res 1
var_ch_ModEffDepth:		.res 1
var_ch_ModEffRate:		.res 2
var_ch_ModInstDepth:	.res 1		;;; ;; ;
var_ch_ModInstRate:		.res 2		;;; ;; ;
var_ch_ModEffWritten:	.res 1
.enum ModEffWritten
	Depth	= %00000001
	RateHi	= %00000010
	RateLo	= %00000100
.endenum

var_ch_FDSVolume:		.res 1		;;; ;; ;
var_ch_ModBias:			.res 1		;;; ;; ;
var_ch_FDSCarrier:		.res 2		;; ;; !! for auto-FM in conjunction with frequency multiplier
var_ch_ModTable:		.res 16
.endif

.if .defined(USE_N163)
var_ch_WavePtrLo:       .res CH_COUNT_N163
var_ch_WavePtrHi:       .res CH_COUNT_N163
var_ch_WaveLen:         .res CH_COUNT_N163			;;; ;; ; MSB is used for N163 Yxx
var_ch_WavePos:         .res CH_COUNT_N163
var_ch_WavePosOld:      .res CH_COUNT_N163			;;; ;; ; overridden by Yxx

var_NamcoChannels:      .res 1                      ; Number of active N163 channels
var_NamcoChannelsReg:   .res 1

var_NamcoInstrument:    .res CH_COUNT_N163
.endif

.if .defined(USE_S5B)								;;; ;; ;
var_Noise_Default:		.res 1						;; ;; !!
var_Noise_Period:		.res 1						; $06
var_Noise_Prev:			.res 1						; cache noise value
var_Pul_Noi:			.res 1						; $07
var_EnvelopeRate:		.res 2						; $0B, $0C
var_EnvelopeType:		.res 1						; $0D
var_EnvelopeAutoShift:	.res CH_COUNT_S5B			;; ;; !!
var_EnvelopeEnabled:	.res 1						;; ;; !! 050B; reduced to 1 because of ZP constraints
var_EnvelopeTrigger:	.res 1						; Hxy issued
.endif												; ;; ;;;

; hack effect variable to zeropage due to low space remaining in BSS when all expansions are enabled
var_ch_Harmonic:		.res EFF_CHANS				;; ;; !! Frequency multiplier
var_ch_PhaseReset:		.res EFF_CHANS				;; ;; !! Phase reset
var_ch_DPCMPhaseReset:	.res 1						;; ;; !! DPCM exclusive phase reset

last_zp_var:			.res 1						; Not used


.segment "BSS"

;
; Driver variables
;

; Song header (necessary to be in order)
var_Song_list:			.res 2						; Song list address
var_Instrument_list:	.res 2						; Instrument list address
.if .defined(USE_DPCM)
var_dpcm_inst_list:		.res 2						; DPCM instruments
var_dpcm_pointers:		.res 2						; DPCM sample pointers
.endif
var_Groove_Table:		.res 2						;;; ;; ; Grooves
var_SongFlags:			.res 1						; Song flags
.if .defined(USE_FDS)
var_Wavetables:			.res 2						; FDS waves
.endif

.if .defined(CHANNEL_CONTROL)
var_Channels:			.res 1						; Channel enable/disable
.endif
var_AllChannels:        .res 1						;;; ;; ; moved from N163
var_EffChannels:        .res 1						; ;; ;;; check against this for DPCM channel

; Track header (necessary to be in order)
var_Frame_List:			.res 2						; Pattern list address
var_Frame_Count:		.res 1						; Number of frames
var_Pattern_Length:		.res 1						; Global pattern length
var_Speed:				.res 1						; Speed setting
var_Tempo:				.res 1						; Tempo setting
var_GroovePointer:		.res 1						;;; ;; ; Groove setting
var_InitialBank:		.res 1

; General
var_PlayerFlags:		.res 1						; Player flags
													; bit 0 = playing
													;;; ;; ; bit 1 = Cxx issued
													; bit 2 - 7 unused
var_Pattern_Pos:		.res 1						; Global pattern row
var_Current_Frame:		.res 1						; Current frame
var_Load_Frame:			.res 1						; 1 if new frame should be loaded

var_Tempo_Accum:		.res 2						; Variables for speed division
var_Tempo_Count:		.res 2						;  (if tempo support is not needed then this can be optimized)
var_Tempo_Dec:			.res 2
var_Tempo_Modulus:		.res 2						;;; ;; ; from 0.4.6
var_Sweep:				.res 1						; This has to be saved
var_VolumeSlideStarted:	.res 1						;; ;; !! Flag to allow simultaneous volume + slide

.if .defined(USE_BANKSWITCH)
;var_Bank:				.res 1
.endif
var_Jump:				.res 1						; If a Jump should be executed
var_Skip:				.res 1						; If a Skip should be executed
.if .defined(ENABLE_ROW_SKIP)
var_SkipTo:				.res 1						; Skip to row number
.endif

var_sequence_ptr:		.res 1
var_sequence_result:	.res 1

;var_enabled_channels:	.res 1

; Channel variables

; General channel variables, used by the pattern reader (all channels)
var_ch_PatternAddrLo:	.res CHANNELS				; Holds current pattern position
var_ch_PatternAddrHi:	.res CHANNELS
.if .defined(USE_BANKSWITCH)
var_ch_Bank:			.res CHANNELS				; Pattern bank
.endif
var_ch_Note:			.res CHANNELS				; Current channel note
var_ch_VolColumn:		.res CHANNELS				; Volume column
var_ch_VolDelay:		.res CHANNELS				;;; ;; ; Delayed channel volume
var_ch_VolDefault:		.res CHANNELS				;;; ;; ; Existing channel volume
var_ch_Delay:			.res CHANNELS				; Delay command
var_ch_NoteCut:			.res CHANNELS
var_ch_NoteRelease:		.res CHANNELS				;;; ;; ; Delayed note release
var_ch_Transpose:		.res CHANNELS				;;; ;; ; Delayed transpose
var_ch_State:			.res CHANNELS				;;; ;; ; overloaded to handle &&
var_ch_FinePitch:		.res CHANNELS				; Fine pitch setting

var_ch_NoteDelay:		.res CHANNELS				; Delay in rows until next note
var_ch_DefaultDelay:	.res CHANNELS				; Default row delay, if exists
var_ch_Trigger:			.res CHANNELS				;;; ;; ; Set for one frame when new note is encountered

; Following is specific to chip channels (2A03, VRC6...)

var_ch_TimerPeriodHi:	.res EFF_CHANS				; Current channel note period
var_ch_TimerPeriodLo:	.res EFF_CHANS
var_ch_PeriodCalcLo:	.res EFF_CHANS 				; Frequency after fine pitch and vibrato has been applied
var_ch_PeriodCalcHi:	.res EFF_CHANS
;var_ch_OutVolume:		.res EFF_CHANS				; Volume for the APU
var_ch_VolSlide:		.res EFF_CHANS				;;; ;; ; Volume slide
var_ch_VolSlideTarget:	.res EFF_CHANS				;; ;; !! Volume slide target
var_ch_DutyDefault:		.res EFF_CHANS				;;; ;; ; Duty cycle / Noise mode
var_ch_DutyCurrent:		.res EFF_CHANS				; ;; ;;; do not rely on bitwise operations

; --- Testing ---
;var_ch_LoopCounter:		.res CHANNELS
; --- Testing ---

; Square 1 & 2 variables
var_ch_Sweep:			.res 2						; Hardware sweep
var_ch_PrevFreqHigh:	.res 2						; Used only by 2A03 pulse channels

; Sequence variables
var_ch_SeqVolume:		.res SFX_WAVE_CHANS * 2		; Sequence 1: Volume
var_ch_SeqArpeggio:		.res SFX_WAVE_CHANS * 2		; Sequence 2: Arpeggio
var_ch_SeqPitch:		.res SFX_WAVE_CHANS * 2		; Sequence 3: Pitch bend
var_ch_SeqHiPitch:		.res SFX_WAVE_CHANS * 2		; Sequence 4: High speed pitch bend
var_ch_SeqDutyCycle:	.res SFX_WAVE_CHANS * 2		; Sequence 5: Duty cycle / Noise Mode
var_ch_Volume:			.res SFX_WAVE_CHANS			; Output volume
var_ch_SequencePtr1:	.res SFX_WAVE_CHANS			; Index pointers for sequences
var_ch_SequencePtr2:	.res SFX_WAVE_CHANS
var_ch_SequencePtr3:	.res SFX_WAVE_CHANS
var_ch_SequencePtr4:	.res SFX_WAVE_CHANS
var_ch_SequencePtr5:	.res SFX_WAVE_CHANS
var_ch_InstType:		.res SFX_WAVE_CHANS			;;; ;; ; Chip type, used for duty conversion
													; ;; ;;; valid since chip type = inst type right now

;var_ch_fixed:			.res SFX_WAVE_CHANS

var_ch_ArpFixed:        .res EFF_CHANS

; Track variables for effects
var_ch_Effect:			.res EFF_CHANS				; Arpeggio & portamento
var_ch_EffParam:		.res EFF_CHANS				; Effect parameter (used by portamento and arpeggio)

var_ch_PortaToHi:		.res EFF_CHANS 				; Portamento
var_ch_PortaToLo:		.res EFF_CHANS
var_ch_ArpeggioCycle:	.res EFF_CHANS				; Arpeggio cycle

var_ch_VibratoPos:		.res EFF_CHANS				; Vibrato
var_ch_VibratoDepth:	.res EFF_CHANS
var_ch_VibratoSpeed:	.res EFF_CHANS
var_ch_TremoloPos:		.res EFF_CHANS				; Tremolo
var_ch_TremoloDepth:	.res EFF_CHANS				; combine these
var_ch_TremoloSpeed:	.res EFF_CHANS
var_ch_TremoloResult:   .res EFF_CHANS
;var_ch_VibratoParam:	.res EFF_CHANS
;var_ch_TremoloParam:	.res EFF_CHANS

ECHO_BUFFER_LENGTH = 3								;;; ;; ; Echo buffer
var_ch_EchoBuffer:		.res (ECHO_BUFFER_LENGTH + 1) * CHANNELS

; End of variable space
last_bss_var:			.res 1						; Not used


.segment "CODE"
.include "longbranch.mac"		;;; ;; ;


; when using FDS + multichip, avoid any data in these addresses:
; $9000 - $9003
; $9010
; $9030
; $A000 - $A002
; $C000
.macro padjmp count ; headerless padding
	.local @end
	.if .defined(USE_ALL)
		.ifndef PACKAGE
			.if count > 3
				jmp @end	
				.repeat count - 3
					nop
				.endrep
			.else
				.repeat count
					nop
				.endrep
			.endif
		.endif
	.endif
	@end:
.endmacro

.macro padjmp_h count ; headered padding
	.local @end
	.if .defined(USE_ALL)
		.ifdef PACKAGE
			.if count > 3
				jmp @end
				.repeat count - 3
					nop
				.endrep
			.else
				.repeat count
					nop
				.endrep
			.endif
		.endif
	.endif
	@end:
.endmacro

.if MULTICHIP		;;; ;; ;
.macro CH_LOOP_START target
	stx var_currentChannel
	lda ft_channel_enable, x
	jeq target
.endmacro
.else
.macro CH_LOOP_START target
	stx var_currentChannel
.endmacro
.endif

.macro CH_LOOP_END target
	ldx var_currentChannel
	inx
	CPX_ALL_CHANNELS
	jne target
.endmacro

.macro CPX_ALL_CHANNELS
.if .defined(USE_ALL)		;;; ;; ;
	cpx #CHANNELS
.elseif .defined(USE_N163)
	cpx var_AllChannels
.else
	cpx #CHANNELS
.endif
.endmacro

; NSF entry addresses
LOAD:
.if .defined(PACKAGE)
	.byte DRIVER_NAME, VERSION_MAJ, VERSION_MIN
.endif
INIT:
	jmp	ft_music_init
PLAY:
	jmp	ft_music_play

.if .defined(CHANNEL_CONTROL)
;;; ;; ; TODO: channel flags for each expansion chip
; Disable channel in X, X = {00 : Square 1, 01 : Square 2, 02 : Triangle, 03 : Noise, 04 : DPCM}
ft_disable_channel:
	lda bit_mask, x
	eor #$FF
	and var_Channels
	sta var_Channels
	rts

; Enable channel in X
ft_enable_channel:
	lda bit_mask, x
	ora var_Channels
	sta var_Channels
	lda #$FF
	cpx #$00
	beq :+
	cpx #$01
	beq :++
	rts
:	sta var_ch_PrevFreqHigh
	rts
:	sta var_ch_PrevFreqHigh + 1
	rts
.endif

; The rest of the code
	.include "init.s"
	.include "player.s"
	.include "effects.s"
	.include "instrument.s"
	.include "apu.s"

.if .defined(USE_VRC6)
	.include "vrc6.s"
.endif
.if .defined(USE_VRC7)
	.include "vrc7.s"
.endif
.if .defined(USE_MMC5)
	.include "mmc5.s"
.endif
.if .defined(USE_FDS)
	.include "fds.s"
.endif
.if .defined(USE_N163)
	.include "n163.s"
.endif
.if .defined(USE_S5B)
	.include "s5b.s"
.endif

ft_bankswitch:
; bankswitch part of song data (frames + patterns, 1 page only)
;	sta $5FFA
	sta $5FFB
	rts
ft_bankswitch2:
; bankswitch DPCM samples (3 pages)
	clc
	sta $5FFC
	adc #$01
	sta $5FFD
	adc #$01
	sta $5FFE
;	adc #$01
;	sta $5FFF
	rts

;
; Channel maps, will be moved to exported data
;

;;; ;; ; ft_channel_map is unnecessary

ft_channel_type: ;; Patch
	.byte CHAN_2A03, CHAN_2A03, CHAN_TRI, CHAN_NOI
.repeat CH_COUNT_MMC5
	.byte CHAN_MMC5
.endrep
.if .defined(USE_VRC6)
	.byte CHAN_VRC6, CHAN_VRC6, CHAN_SAW
.endif
.repeat CH_COUNT_N163		; 0CC: check
	.byte CHAN_N163
.endrep
.repeat CH_COUNT_FDS
	.byte CHAN_FDS
.endrep
.repeat CH_COUNT_S5B
	.byte CHAN_S5B
.endrep
.repeat CH_COUNT_VRC7
	.byte CHAN_VRC7
.endrep
.if .defined(USE_DPCM)
	.byte CHAN_DPCM
.endif

.if MULTICHIP		;;; ;; ;
	.if .defined(USE_AUX_DATA) .and .defined(USE_ALL)
		.include "../enable_ext.s"
	.else
		ft_channel_enable: ;; Patch
		.if .defined(PACKAGE)
			;; ;; !! patched by the tracker
			.res CHANNELS, 0
		.else
				.byte 1, 1, 1, 1
			.repeat CH_COUNT_MMC5
				.byte .defined(USE_MMC5)
			.endrep
			.repeat CH_COUNT_VRC6
				.byte .defined(USE_VRC6)
			.endrepeat
			.repeat CH_COUNT_N163		; 0CC: check
				.byte .defined(USE_N163)
			.endrep
			.repeat CH_COUNT_FDS
				.byte .defined(USE_FDS)
			.endrep
			.repeat CH_COUNT_S5B
				.byte .defined(USE_S5B)
			.endrep
			.repeat CH_COUNT_VRC7
				.byte .defined(USE_VRC7)
			.endrep
			.if .defined(USE_DPCM)
				.byte 1
			.endif
		.endif
	.endif
.endif

bit_mask:		;;; ;; ; general-purpose bit mask
.repeat 8, i
	.byte 1 << i
.endrep

; Include period tables
.if .defined(USE_AUX_DATA)
	; Period tables are overwritten when detune settings are present.
	.include "../periods.s"
.else
	.include "periods.s"
.endif

;;; ;; ; Include vibrato table
.if .defined(USE_AUX_DATA)
	; Vibrato tables are overwritten depending on old/new vibrato mode.
	.include "../vibrato.s"
.else
	.include "vibrato.s"
.endif

LIMIT_PERIOD_2A03 = $7FF
LIMIT_PERIOD_VRC6 = $FFF
; VRC7: period is between 0 to (1 << (VRC7_PITCH_RESOLUTION + 9)) - 1 or $7FF
LIMIT_PERIOD_VRC7 = LIMIT_PERIOD_2A03
LIMIT_PERIOD_N163 = $FFFF
LIMIT_PERIOD_LINEAR = (95<<5)

ft_limit_freq_lo:
	.byte >LIMIT_PERIOD_2A03		; 2A03
	.byte >LIMIT_PERIOD_2A03		; 2A03
	.byte >0						; 2A03 noise
	.byte >0						; 2A03 dpcm
	.byte >LIMIT_PERIOD_VRC6		; VRC6
	.byte >LIMIT_PERIOD_VRC6		; VRC6
	.byte >LIMIT_PERIOD_2A03		; VRC7
	.byte >LIMIT_PERIOD_VRC6		; FDS
	.byte >LIMIT_PERIOD_2A03		; MMC5
	.byte >LIMIT_PERIOD_N163		; N163
	.byte >LIMIT_PERIOD_VRC6		; S5B
ft_limit_freq_hi:
	.byte <LIMIT_PERIOD_2A03		; 2A03
	.byte <LIMIT_PERIOD_2A03		; 2A03
	.byte <0						; 2A03 noise
	.byte <0						; 2A03 dpcm
	.byte <LIMIT_PERIOD_VRC6		; VRC6
	.byte <LIMIT_PERIOD_VRC6		; VRC6
	.byte <LIMIT_PERIOD_2A03		; VRC7
	.byte <LIMIT_PERIOD_VRC6		; FDS
	.byte <LIMIT_PERIOD_2A03		; MMC5
	.byte <LIMIT_PERIOD_N163		; N163
	.byte <LIMIT_PERIOD_VRC6		; S5B


;
; An example of including music follows
;

; The label that contains a pointer to the music data
;  A simple way to handle multiple songs is to move this
;  to RAM and setup a table of pointers to music data
ft_music_addr:
	.addr * + 2					; This is the point where music data is stored

.if .defined(INC_MUSIC)
		; Include music
	.if .defined(INC_MUSIC_ASM)
		; Included assembly file music, DPCM included
		.include "../music.asm"
	.else
		; Binary chunk music
		.incbin "../music.bin"
		.if .defined(USE_DPCM)
			.segment "DPCM"				; DPCM samples goes here
			.incbin "../samples.bin"
		.endif
	.endif
.endif
