;
; Sunsoft 5B expansion sound
;

ft_init_s5b:
	lda #$40
	sta var_ch_DutyDefault + S5B_OFFSET
	sta var_ch_DutyDefault + S5B_OFFSET + 1
	sta var_ch_DutyDefault + S5B_OFFSET + 2
	lda #$FF
	sta var_Noise_Prev
	sta var_AutoEnv_Channel
	lda #$00
	sta var_EnvelopeRate
	sta var_EnvelopeRate + 1
	lda #$07
	sta $C000
	lda #%00111000
	sta var_Pul_Noi
	sta $E000		; see n163.s
	rts

ft_update_s5b:
	lda var_PlayerFlags
	bne :+
	ldx #$08
	stx $C000
	sta $E000
	inx
	stx $C000
	sta $E000
	inx
	stx $C000
	sta $E000
	rts
:
	ldx #$00
	stx var_Pul_Noi
@UpdateNoise:
	; a = var_ch_SequencePtr5[S5B_OFFSET + x]
	; if a == #$FF goto :+

	lda var_ch_SequencePtr5 + S5B_OFFSET, x
	cmp #$FF
	beq :+

	lda var_ch_DutyCurrent + S5B_OFFSET, x
	bpl :+									; no noise
	and #$1F
	sta var_Noise_Period
:	inx
	cpx #CH_COUNT_S5B
	bcc @UpdateNoise

	ldx #(CH_COUNT_S5B - 1)
@UpdateNoiseMask:
	asl var_Pul_Noi
	lda var_ch_DutyCurrent + S5B_OFFSET, x
	bmi :+
	inc var_Pul_Noi
:	dex
	bpl @UpdateNoiseMask

	ldx #(CH_COUNT_S5B - 1)
@UpdateToneMask:
	asl var_Pul_Noi
	lda var_ch_DutyCurrent + S5B_OFFSET, x
	and #$40
	bne :+
	inc var_Pul_Noi
:	dex
	bpl @UpdateToneMask

.if .defined(USE_LINEARPITCH)		;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :+
	jsr ft_load_ntsc_table
	ldx #S5B_OFFSET
	jsr ft_linear_fetch_pitch
	jsr ft_linear_fetch_pitch
	jsr ft_linear_fetch_pitch
:
.endif								; ;; ;;;
	ldx #$00
	ldy #$00
@ChannelLoop:
	lda var_ch_Note + S5B_OFFSET, x				; Kill channel if note = off
	bne :+
	txa
	ora #$08
;	clc
;	adc #$08
	sta $C000
	lda #$00
	sta $E000
	iny
	iny
	bpl @S5B_next ; always
	; Load volume
:	txa
	ora #$08
;	clc
;	adc #$08
	sta $C000
	lda var_ch_VolColumn + S5B_OFFSET, x
	beq @StoreVolume
	lsr a
	lsr a
	lsr a
	sta var_Temp
	lda var_ch_Volume + S5B_OFFSET, x
	beq @StoreVolume
	clc
	adc var_Temp
	sec
	sbc #$0F
	sec
	sbc var_ch_TremoloResult + S5B_OFFSET, x
	bpl :+
	lda #$00
:	bne :+
	lda var_ch_VolColumn + S5B_OFFSET, x
	beq :+
	lda #$01
:
@StoreVolume:
	; Volume / envelope enable
	sta var_Temp
	lda var_ch_DutyCurrent + S5B_OFFSET, x
	and #$20								; E
	lsr
	ora var_Temp
	sta $E000
	; Frequency
	inc var_ch_PeriodCalcLo + S5B_OFFSET, x		; correction
	bne :+
	inc var_ch_PeriodCalcHi + S5B_OFFSET, x
:	sty $C000
	iny
	lda var_ch_PeriodCalcLo + S5B_OFFSET, x
	sta $E000
	sty $C000
	iny
	lda var_ch_PeriodCalcHi + S5B_OFFSET, x
	sta $E000
	
	lda var_ch_DutyCurrent + S5B_OFFSET, x
	and #$20
	beq @S5B_next
	lda var_ch_Trigger + S5B_OFFSET, x
	beq @S5B_next
	sta var_EnvelopeTrigger ; should be 1 anyway
@S5B_next:
	inx
	cpx #CH_COUNT_S5B
	bcs :+
	jmp @ChannelLoop
:
	; Global variables
	ldx #$06
	lda var_Noise_Period
	eor #$1F
	cmp var_Noise_Prev
	beq :+
	sta var_Noise_Prev
	stx $C000
	sta $E000
:	inx
	stx $C000
	lda var_Pul_Noi
	sta $E000

	ldx var_AutoEnv_Channel
	bpl @AutoEnv
	ldx #$0B
	stx $C000
	lda var_EnvelopeRate
	sta $E000
	inx
	stx $C000
	lda var_EnvelopeRate + 1
	sta $E000
	jmp @Finished

@AutoEnv:
	lda var_ch_PeriodCalcLo, x
	sta var_Temp16
	lda var_ch_PeriodCalcHi, x
	sta var_Temp16 + 1

	lda var_EnvelopeType		;;; ;; ; 050B
	lsr
	lsr
	lsr
	lsr
	cmp #$08
	beq @Write
	bcc @LowerOctave
@RaiseOctave:
	sec
	sbc #$08
	tay
:	lsr var_Temp16 + 1
	ror var_Temp16
	dey
	bne :-
	bcc @Write
	inc var_Temp16
	bne @Write
	inc var_Temp16 + 1
	bne @Write ; always
@LowerOctave:
	sta var_Temp
	sec
	lda #$08
	sbc var_Temp
	tay
:	asl var_Temp16
	rol var_Temp16 + 1
	dey
	bne :-
@Write:
	ldx #$0B
	stx $C000
	lda var_Temp16
	sta $E000
	inx
	stx $C000
	lda var_Temp16 + 1
	sta $E000

@Finished:
	lda var_EnvelopeTrigger
	beq :+
	lda #$00
	sta var_EnvelopeTrigger
	lda #$0D
	sta $C000
	lda var_EnvelopeType
	and #$0F
	sta $E000
:	rts
