; Update the instrument for channel X
;
; I might consider storing the sequence address variables in ZP??
;
ft_run_instrument:
.if .defined(USE_VRC7)
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne :+
	rts
:
.endif
	; Volume
	;
	lda var_ch_SeqVolume + SFX_WAVE_CHANS, x	; High part of address = 0 mean sequence is disabled
	beq @SkipVolumeUpdate
	sta var_Temp_Pointer + 1
	lda var_ch_SeqVolume, x					; Store the sequence address in a zp variable
	sta var_Temp_Pointer
	lda var_ch_SequencePtr1, x				; Sequence item index
	cmp #$FF
	beq @SkipVolumeUpdate					; Skip if end is reached
	jsr ft_run_sequence						; Run an item in the sequence
	sta var_ch_SequencePtr1, x				; Store new index
	lda var_sequence_result					; Take care of the result
	sta var_ch_Volume, x
@SkipVolumeUpdate:

	; Arpeggio
	;
	lda var_ch_SeqArpeggio + SFX_WAVE_CHANS, x
	bne :+		;;; ;; ; too long ; ;; ;;;
	jmp @SkipArpeggioUpdate
:	sta var_Temp_Pointer + 1
	lda var_ch_SeqArpeggio, x
	sta var_Temp_Pointer
	lda var_ch_SequencePtr2, x
	cmp #$FF
	bne :+		;;; ;; ; too long ; ;; ;;;
	jmp @RestoreArpeggio;@SkipArpeggioUpdate
:	jsr ft_run_sequence
	sta var_ch_SequencePtr2, x
	lda var_ch_Note, x					; No arp if no note
	bne :+		;;; ;; ; too long ; ;; ;;;
	jmp @SkipArpeggioUpdate
:
	ldy #$03
	lda (var_Temp_Pointer), y
	bne :+		;;; ;; ;
	jmp @Absolute
:	cmp #$01
	bne :+
	jmp	@Fixed
:	cmp #$02
	bne :+
	jmp @Relative				; cmp #$03 bne :+ jmp @SchemeArp
:
@SchemeArp:
	; 0xy Scheme
	; adds x offset from 0xy effect when b6 is on, y when b7 is on
	lda var_sequence_result
	and #$3F					; \x29\x3f\xc9\x25
	cmp #$25
	bmi :+						; limit sequence range in [-27,36]
	sec
	sbc #$40
:	clc
	adc var_ch_Note, x			; root increment
	tay							; keep note in Y
	lda var_ch_Effect, x
	cmp #EFF_ARPEGGIO
	bne :+						; 0xy not in effect
	lda var_sequence_result
	and #$C0
	clc
	rol a
	rol a
	rol a						; 00 = root, 01 = x, 02 = y, 03 = -y
	bne :++						; root transposition
:	tya
	jmp @Limit
:	cmp #$01					; adding high nybble
	bne :+
	lda var_ch_EffParam, x
	lsr a
	lsr a
	lsr a
	lsr a
	bpl :+++ ; always
:	cmp #$02					; adding low nybble
	bne :+
	lda var_ch_EffParam, x
	and #$0F
	bpl :++
:	lda var_ch_EffParam, x		; subtracting low nybble
	and #$0F
	eor #$FF
	clc
	adc #$01
:	sty var_sequence_result		; safe to overwrite
	clc
	adc var_sequence_result
	jmp @Limit
@Relative:
	; Relative
	clc
	lda var_ch_Note, x
	adc var_sequence_result
	bpl :++
	; if negative, check for overflow
	clc
	lda var_sequence_result
	bpl :+
	lda #$01		; if underflow, clamp to #$01
	jmp :++
:
	lda #$60		; if overflow, clamp to #$60
:	jsr ft_limit_note
	sta var_ch_Note, x
	jmp @ArpDone
@Fixed:
	; Fixed
	lda var_sequence_result
	clc
	adc #$01
	jmp @Limit
@Absolute:
	; Absolute
	clc
	lda var_ch_Note, x
	adc var_sequence_result
	bpl @Limit
	; if negative, check for overflow
	clc
	lda var_sequence_result
	bpl :+
	adc var_ch_Note, x
	jmp @Limit		; handles underflow
:	
	lda #$60		; if overflow, clamp to #$60
@Limit:
	jsr ft_limit_note
@ArpDone:
	jsr ft_translate_freq_only
	lda #$01
	sta var_ch_ArpFixed, x
	jmp @SkipArpeggioUpdate

@RestoreArpeggio:
	ldy #$03
	lda (var_Temp_Pointer), y
	beq @SkipArpeggioUpdate
	lda var_ch_ArpFixed, x
	beq @SkipArpeggioUpdate
	lda var_ch_Note, x			   ; No arp if no note
	jsr ft_translate_freq_only
	lda #$00
	sta var_ch_ArpFixed, x

@SkipArpeggioUpdate:

	; Pitch bend
	;
	lda var_ch_SeqPitch + SFX_WAVE_CHANS, x
	beq @SkipPitchUpdate
	sta var_Temp_Pointer + 1
	lda var_ch_SeqPitch, x
	sta var_Temp_Pointer
	lda var_ch_SequencePtr3, x
	cmp #$FF
	beq @SkipPitchUpdate
	jsr ft_run_sequence
	sta var_ch_SequencePtr3, x
	
	ldy #$03		;;; ;; ; 050B
	lda (var_Temp_Pointer), y
	beq :+
	lda var_ch_Note, x
	jsr ft_translate_freq_only

	; Check this
:	clc
	lda var_sequence_result
	adc var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodLo, x
	lda var_sequence_result
	bpl @NoNegativePitch
	lda #$FF
	bmi @LoadLowPitch
@NoNegativePitch:
	lda #$00
@LoadLowPitch:
	adc var_ch_TimerPeriodHi, x
	sta var_ch_TimerPeriodHi, x
	jsr ft_limit_freq
	; ^^^^^^^^^^

	; Save pitch
@SkipPitchUpdate:
	; HiPitch bend
	;
	lda var_ch_SeqHiPitch + SFX_WAVE_CHANS, x
	beq @SkipHiPitchUpdate
	sta var_Temp_Pointer + 1
	lda var_ch_SeqHiPitch, x
	sta var_Temp_Pointer
	lda var_ch_SequencePtr4, x
	cmp #$FF
	beq @SkipHiPitchUpdate
	jsr ft_run_sequence
	sta var_ch_SequencePtr4, x

	; Check this
	lda var_sequence_result
	sta var_Temp16
	rol a
	bcc @AddHiPitch
	lda #$FF
	sta var_Temp16 + 1
	jmp @StoreHiPitch
@AddHiPitch:
	lda #$00
	sta var_Temp16 + 1
@StoreHiPitch:
	ldy #$04
:	clc
	rol var_Temp16 						; multiply by 2
	rol var_Temp16 + 1
	dey
	bne :-

	clc
	lda var_Temp16
	adc var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodLo, x
	lda var_Temp16 + 1
	adc var_ch_TimerPeriodHi, x
	sta var_ch_TimerPeriodHi, x
	jsr ft_limit_freq
	; ^^^^^^^^^^

@SkipHiPitchUpdate:
	; Duty cycle/noise mode
	;
	lda var_ch_SeqDutyCycle + SFX_WAVE_CHANS, x
	beq @SkipDutyUpdate
	sta var_Temp_Pointer + 1
	lda var_ch_SeqDutyCycle, x
	sta var_Temp_Pointer
	lda var_ch_SequencePtr5, x
	cmp #$FF
	beq @SkipDutyUpdate
	jsr ft_run_sequence
	sta var_ch_SequencePtr5, x
@ConvertDuty:		;;; ;; ;
	lda ft_channel_type, x
	cmp #CHAN_VRC6
	beq @ToVRC6
;	cmp #CHAN_N163
;	beq @ToN163
	cmp #CHAN_S5B
	beq @ToS5B
	cmp #CHAN_MMC5
	beq :+
	cmp #CHAN_2A03
	bne @NoConvert
:
	ldy var_sequence_result
	lda var_ch_InstType, x
	cmp #CHAN_S5B
	bne :+
	lda #$02
	bpl @DoneConvert ; always
:	cmp #CHAN_VRC6
	bne :+
	lda ft_duty_vrc6_to_2a03, y
	bpl @DoneConvert ; always
:	jmp @NoConvert
@ToVRC6:
	ldy var_sequence_result
	lda var_ch_InstType, x
	cmp #CHAN_S5B
	bne :+
	lda #$07
	bpl @DoneConvert ; always
:	cmp #CHAN_2A03
	bne :+
	lda ft_duty_2a03_to_vrc6, y
	bpl @DoneConvert ; always
:	jmp @NoConvert
@ToS5B:
	lda var_ch_InstType, x
	cmp #CHAN_S5B
	beq @NoConvert
	lda #$40
	bpl @DoneConvert ; always
@NoConvert:
	lda var_sequence_result
@DoneConvert:		; ;; ;;;
	sta var_ch_DutyCurrent, x
.if .defined(USE_S5B)
;	if (Value & S5B_MODE_NOISE)
;		pChan->SetNoiseFreq(Value & 0x1F);
	bpl :+
	and #$1F
	sta var_Noise_Period
:
.endif
	jmp @LoadWave
	; Save pitch
@SkipDutyUpdate:
.if .defined(USE_N163)
	lda var_ch_Trigger, x
	beq @Finish
.endif
@LoadWave:
.if .defined(USE_N163)
	lda ft_channel_type, x
	cmp #CHAN_N163
	bne @Finish
	jsr ft_n163_load_wave2
.endif
@Finish:
	rts


;
; Process a sequence, next position is returned in A
;
; In: A = Sequence index
; Out: A = New sequence index
;
ft_run_sequence:
	clc
	adc #$04						; Offset is 4 items
	tay
	lda (var_Temp_Pointer), y
	sta var_sequence_result
	dey
	dey
	dey ; (remove)
	tya
	ldy #$00						; Check if halt point
	cmp (var_Temp_Pointer), y
	beq @HaltSequence
	ldy #$02						; Check release point
	cmp (var_Temp_Pointer), y
	beq @ReleasePoint
	rts
@HaltSequence:						; Stop the sequence
	iny
	lda (var_Temp_Pointer), y		; Check loop point
	cmp #$FF
	bne @LoopSequence
;	lda #$FF						; Disable sequence by loading $FF into length
	rts
@LoopSequence:						; Just return A
	pha
	lda var_ch_State, x
	and #STATE_RELEASE				;;; ;; ;
	bne :++
:	pla
	rts								; Return new index
:	ldy #$02						; Check release point
	lda (var_Temp_Pointer), y
	beq :--							; Release point not found, loop
	sta var_Temp					;;; ;; ;
	dec var_Temp
	pla								; Release point found, don't loop
	cmp var_Temp
	bcs :+							; ;; ;;;
	lda #$FF
:	rts
@ReleasePoint:						; Release point has been reached
	sta var_Temp					; Save index
	lda var_ch_State, x
	and #STATE_RELEASE				;;; ;; ;
	bne @Releasing
	dey
	lda (var_Temp_Pointer), y		; Check loop point
	cmp #$FF
	beq :++
	dec var_Temp					;;; ;; ;
	cmp var_Temp
	bcs :+
	bcc @LoopSequence ; always
:	inc var_Temp
:	lda var_Temp					; ;; ;;;
	sec								; Step back one step
	sbc #$01
	rts
@Releasing:                         ; Run release sequence
	lda var_Temp
	rts

.macro release sequence, sequence_pointer
	lda sequence + SFX_WAVE_CHANS, x
	beq :+;+
	sta var_Temp_Pointer + 1
	lda sequence, x
	sta var_Temp_Pointer
	ldy #$02
	lda (var_Temp_Pointer), y
	beq :+							; Release not available
	sec
	sbc #$01
	sta sequence_pointer, x
:
.endmacro

; Called on note release instruction
;
ft_instrument_release:
	tya
	pha

	release var_ch_SeqVolume, var_ch_SequencePtr1
	release var_ch_SeqArpeggio, var_ch_SequencePtr2
	release var_ch_SeqPitch, var_ch_SequencePtr3
	release var_ch_SeqHiPitch, var_ch_SequencePtr4
	release var_ch_SeqDutyCycle, var_ch_SequencePtr5

	pla
	tay
	rts

; Reset instrument sequences
;
ft_reset_instrument:

.if .defined(USE_FDS)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_FDS
	bne :+						; ;; ;;;
	lda var_ch_ModDelay
	sta var_ch_ModDelayTick
;	lda #$00
;	sta $4085
;	lda #$80
;	sta $4087
;	rts
:
.endif

	jsr ft_get_hold_clear		;;; ;; ;
	bne :+
.if 0
	lda var_VolTemp
	sta var_ch_Volume, x
	lda #$00
	sta var_ch_ArpeggioCycle, x
.endif

	lda #$00
	sta var_ch_SequencePtr1, x
	sta var_ch_SequencePtr2, x
	sta var_ch_SequencePtr3, x
	sta var_ch_SequencePtr4, x
	sta var_ch_SequencePtr5, x
:	rts

; Macros


; Macro used to load instrument envelopes
.macro load_inst seq_addr, seq_ptr

	ror var_Temp3
	bcc	:++
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp_Pointer), y
	adc ft_music_addr
	sta var_Temp16
	iny
	lda (var_Temp_Pointer), y
	adc ft_music_addr + 1
	sta var_Temp16 + 1
	iny
.else
	lda (var_Temp_Pointer), y
	sta var_Temp16
	iny
	lda (var_Temp_Pointer), y
	sta var_Temp16 + 1
	iny
.endif

	lda var_Temp16
	cmp seq_addr, x
	bne :+
	lda var_Temp16 + 1
	cmp seq_addr + SFX_WAVE_CHANS, x
	bne :+

	; Both equal, do not touch anything
	jmp :+++

:	lda var_Temp16
	sta seq_addr, x
	lda var_Temp16 + 1
	sta seq_addr + SFX_WAVE_CHANS, x
	lda #$00
	sta seq_ptr, x
	jmp :++		; branch always

:	lda #$00
	sta seq_addr, x
	sta seq_addr + SFX_WAVE_CHANS, x
:

.endmacro

;
; Load instrument (y = saved in var_Temp)
;
; A = instrument number
;
ft_load_instrument:

	; Instrument_pointer_list + a => instrument_address
	; instrument_address + ft_music_addr => instrument_data

	sta var_Temp2	;;; ;; ; used by N163

	; Get the instrument data pointer
	sty var_Temp
	ldy #$00
	clc
	adc var_Instrument_list
	sta var_Temp16
	tya
	adc var_Instrument_list + 1
	sta var_Temp16 + 1

	; Get the instrument
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp16), y
	adc ft_music_addr
	sta var_Temp_Pointer
	iny
	lda (var_Temp16), y
	adc ft_music_addr + 1
	sta var_Temp_Pointer + 1
.else
	lda (var_Temp16), y
	sta var_Temp_Pointer
	iny
	lda (var_Temp16), y
	sta var_Temp_Pointer + 1
.endif
	dey		;;; ;; ;

	; Jump to the instrument setup routine
	;;; ;; ; only vrc7 does not use sequence instrument
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne :+
	jmp ft_load_instrument_vrc7
:	; continue

; Load 2A03 instrument
; ft_load_instrument_2a03:
	; Read instrument data, var_Temp_Pointer points to instrument data
	lda (var_Temp_Pointer), y		;;; ;; ; instrument type
	sta var_ch_InstType, x
	iny								; ;; ;;;
	lda (var_Temp_Pointer), y		; sequence switch
	sta var_Temp3
	iny

	load_inst var_ch_SeqVolume, var_ch_SequencePtr1
	load_inst var_ch_SeqArpeggio, var_ch_SequencePtr2
	load_inst var_ch_SeqPitch, var_ch_SequencePtr3
	load_inst var_ch_SeqHiPitch, var_ch_SequencePtr4
	load_inst var_ch_SeqDutyCycle, var_ch_SequencePtr5

.if .defined(USE_FDS) || .defined(USE_N163)		;;; ;; ;
	lda var_ch_InstType, x
.if .defined(USE_FDS)
	cmp #CHAN_FDS
	bne :+
	jmp ft_load_inst_extra_fds
:
.endif
.if .defined(USE_N163)
	cmp #CHAN_N163
	bne :+
	jmp ft_load_inst_extra_n163
:
.endif
.endif											; ;; ;;;
.ifndef USE_VRC7
ft_load_instrument_vrc7:
.endif

	ldy var_Temp		;;; ;; ;
	rts

; Make sure the period doesn't exceed max or min
ft_limit_freq:

	; Jump to the instrument setup routine
	lda ft_channel_type, x
	asl		;;; ;; ;
	tay
	lda ft_limit_pointers, y
	sta var_Temp16
	iny
	lda ft_limit_pointers, y
	sta var_Temp16 + 1
	ldy #$00
	jmp (var_Temp16)


ft_limit_pointers:			;;; ;; ; 0CC: optimize this
	.word ft_limit_period_2a03		; 2A03
	.word ft_limit_period_2a03		; 2A03
	.word ft_limit_period_no		; 2A03 noise
	.word ft_limit_period_no		; 2A03 dpcm
	.word ft_limit_period_vrc6		; VRC6
	.word ft_limit_period_vrc6		; VRC6
	.word ft_limit_period_no		; VRC7
	.word ft_limit_period_vrc6		; FDS
	.word ft_limit_period_2a03		; MMC5
	.word ft_limit_period_no		; N163
	.word ft_limit_period_vrc6		;;; ;; ; S5B

ft_limit_period_no:
	rts

; 2A03: period is between 0 to $7FF
ft_limit_period_2a03:
	lda var_ch_TimerPeriodHi, x
	bmi @LimitMin
	cmp #$08
	bcc @NoLimit
	lda #$07
	sta var_ch_TimerPeriodHi, x
	lda #$FF
	sta var_ch_TimerPeriodLo, x
@NoLimit:
	rts
@LimitMin:
	lda #$00
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
	rts

; VRC6: period is between 0 to $FFF
ft_limit_period_vrc6:
	lda var_ch_TimerPeriodHi, x
	bmi @LimitMin
	cmp #$10
	bcc @NoLimit
	lda #$0F
	sta var_ch_TimerPeriodHi, x
	lda #$FF
	sta var_ch_TimerPeriodLo, x
@NoLimit:
	rts
@LimitMin:
	lda #$00
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
	rts

.if 0

	lda var_ch_TimerPeriodHi, x
	bmi @LimitMin						; period < 0
.if .defined(USE_VRC6)
	pha									;;; ;; ;
	lda ft_channel_type, x
	cmp #CHAN_VRC6
	beq :+
	cmp #CHAN_SAW
	beq :+
	pla
	bpl :++ ; always
:	pla									; ;; ;;;
	cmp #$10							; period > $FFF
	bcc @NoLimit
	lda #$0F
	sta var_ch_TimerPeriodHi, x
	lda #$FF
	sta var_ch_TimerPeriodLo, x
	rts
:
.endif
.if .defined(USE_FDS)
	cpx #FDS_OFFSET
	bne :+
	cmp #$11							; period > $1000?
	bcc @NoLimit
	lda #$10
	sta var_ch_TimerPeriodHi, x
	lda #$FF
	sta var_ch_TimerPeriodLo, x
	rts
:
.endif
	cmp #$08							; period > $7FF
	bcc @NoLimit
	lda #$07
	sta var_ch_TimerPeriodHi, x
	lda #$FF
	sta var_ch_TimerPeriodLo, x
@NoLimit:
	rts
@LimitMin:
	lda #$00
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
	rts

.endif