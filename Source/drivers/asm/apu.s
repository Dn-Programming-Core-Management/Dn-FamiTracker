;
; Updates the APU registers. x and y are free to use
;

; calculates FLOOR(((var_Temp + 1) * (var_Temp2 + 1) - 1) / 16)
; 4 effective bits for var_Temp2, each block adds one
.if .defined(USE_VRC6) || .defined(USE_FDS)
ft_multiply_volume:		;;; ;; ; 050B
	lda var_Temp
	lsr var_Temp2
	bcs :+
	lsr a
:
	lsr var_Temp2
	bcc :+
	adc var_Temp
:	lsr a

	lsr var_Temp2
	bcc :+
	adc var_Temp
:	lsr a

	lsr var_Temp2
	bcc :+
	adc var_Temp
:	lsr a

	beq :+
	rts
:	lda var_Temp
	ora var_ch_Volume, x
	beq :+
	lda #$01					; Round up to 1
:	rts
.endif

ft_update_2a03:
	lda var_PlayerFlags
	bne @Play
	lda #$00					; Kill all channels
	sta $4015
	rts
@KillSweepUnit:					; Reset sweep unit to avoid strange problems
	lda #$C0
	sta $4017
	lda #$40
	sta $4017
	rts
@Play:
.if .defined(USE_LINEARPITCH)		;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq @End
.if .defined(PAL_PERIOD_TABLE)
	lda var_SongFlags
	and #FLAG_USEPAL
	bne :+
	jsr ft_load_ntsc_table
	jmp @TableLoaded
:	jsr ft_load_pal_table
.else
	jsr ft_load_ntsc_table
.endif
@TableLoaded:
	ldx #APU_OFFSET
	jsr ft_linear_fetch_pitch
	jsr ft_linear_fetch_pitch
	jsr ft_linear_fetch_pitch
@End:
.endif								; ;; ;;;
	ldx #$00
	ldy #$00
; ==============================================================================
;  Square 1 / 2
; ==============================================================================
@Square:
.if .defined(CHANNEL_CONTROL)
	lda bit_mask, x
	and var_Channels
	bne :+
	jmp @DoneSquare
:
.endif
	lda var_ch_Note, x			; Kill channel if note = off
	bne :+						; branch
	jmp @KillSquare
:

	; Calculate volume
.if 0
	ldx #$00
	jsr ft_get_volume
	beq @KillSquare
.endif
	; Calculate volume
	lda var_ch_LengthCounter + APU_OFFSET, x	;;; ;; ;
	and #$01
	beq :+
	lda var_ch_VolColumn + APU_OFFSET, x		; do not automatically kill channel when hardware envelope is enabled
	asl a
	and #$F0
	ora var_ch_Volume + APU_OFFSET, x
	tay
	lda ft_volume_table, y						; ignore tremolo
	bpl @DoneVolumeSquare						; always ; ;; ;;;
:	lda var_ch_VolColumn + APU_OFFSET, x		; Kill channel if volume column = 0
	asl a
	bne :+
	jmp @KillSquare
:	and #$F0
	sta var_Temp
	lda var_ch_Volume + APU_OFFSET, x
	bne :+
	jmp @KillSquare
:	ora var_Temp
	tay
	lda ft_volume_table, y
	sec
	sbc var_ch_TremoloResult + APU_OFFSET, x
	bpl :+
	lda #$00
:   bne :+
	lda var_ch_VolColumn + APU_OFFSET, x
	beq :+
	lda #$01
:
@DoneVolumeSquare:
	; Write to registers
	pha
	lda var_ch_DutyCurrent + APU_OFFSET, x
	and #$03
	tay
	pla
	ora ft_duty_table, y						; Add volume
	sta var_Temp								;;; ;; ; allow length counter and envelope
	txa
	asl a
	asl a
	tay
	lda var_ch_LengthCounter + APU_OFFSET, x
	and #$03
	eor #$03
	asl a
	asl a
	asl a
	asl a
	ora var_Temp								; ;; ;;;
	sta $4000, y								; $4000/4004
	iny
	; Period table isn't limited to $7FF anymore
	lda var_ch_PeriodCalcHi + APU_OFFSET, x
	and #$F8
	beq :+
	lda #$07
	sta var_ch_PeriodCalcHi + APU_OFFSET, x
	lda #$FF
	sta var_ch_PeriodCalcLo + APU_OFFSET, x
:
	lda var_ch_Sweep + APU_OFFSET, x			; Check if sweep is active
	beq @NoSquareSweep
	and #$80
	beq @DoneSquare								; See if sweep is triggered, if then don't touch sound registers until next note

	lda var_ch_Sweep + APU_OFFSET, x			; Trigger sweep
	sta $4000, y								; $4001/4005
	iny
	and #$7F
	sta var_ch_Sweep + APU_OFFSET, x

	jsr @KillSweepUnit

	lda var_ch_PeriodCalcLo + APU_OFFSET, x
	sta $4000, y								; $4002/4006
	iny
	lda var_ch_PeriodCalcHi + APU_OFFSET, x
	sta $4000, y								; $4003/4007
	lda #$FF
	sta var_ch_PrevFreqHigh + APU_OFFSET, x

	jmp @DoneSquare

@NoSquareSweep:									; No Sweep
	lda #$08
	sta $4000, y								; $4001/4005
	iny
	;jsr @KillSweepUnit							; test
	lda var_ch_PeriodCalcLo + APU_OFFSET, x
	sta $4000, y								; $4002/4006
	iny
	lda var_ch_LengthCounter + APU_OFFSET, x	;;; ;; ;
	and #$03
	beq :+
	lda var_ch_Trigger + APU_OFFSET, x
	beq @DoneSquare
	bne :++
:	lda var_ch_PeriodCalcHi + APU_OFFSET, x
	cmp var_ch_PrevFreqHigh + APU_OFFSET, x
	beq @DoneSquare
	sta var_ch_PrevFreqHigh + APU_OFFSET, x
:	lda var_ch_LengthCounter + APU_OFFSET, x
	and #$F8
	ora var_ch_PeriodCalcHi + APU_OFFSET, x
	sta $4000, y								; $4003/4007
	jmp @DoneSquare
	
@KillSquare:
	lda #$30
	sta $4000, y								; $4000/4004

@DoneSquare:
	lda var_ch_PhaseReset + APU_OFFSET, x
	bne @SquarePhaseReset
	inx
	cpx #$02
	bcs :+
	ldy #$04
	;txa
	;asl a
	;asl a
	;tay
	jmp @Square
:	jmp @Triangle

@SquarePhaseReset:
	lda #$00
	sta var_ch_PhaseReset + APU_OFFSET, x
	lda var_ch_LengthCounter + APU_OFFSET, x	;;; ;; ;
	and #$03
	beq :+
	lda var_ch_Trigger + APU_OFFSET, x
	beq @DoneSquare
	bne :++
:	lda var_ch_PeriodCalcHi + APU_OFFSET, x
	sta var_ch_PrevFreqHigh + APU_OFFSET, x
:	lda var_ch_LengthCounter + APU_OFFSET, x
	and #$F8
	ora var_ch_PeriodCalcHi + APU_OFFSET, x
	sta $4000, y								; $4003/4007
	rts

; ==============================================================================
;  Triangle
; ==============================================================================
@Triangle:
.if .defined(CHANNEL_CONTROL)
	lda var_Channels
	and #$04
	beq @Noise
.endif

	lda var_ch_Volume + APU_TRI
	beq @KillTriangle
	lda var_ch_VolColumn + APU_TRI
	beq @KillTriangle
	lda var_ch_Note + APU_TRI
	beq @KillTriangle
	lda var_ch_LengthCounter + APU_TRI	;;; ;; ;
	and #%00000011
	beq :+								; branch if no length counter and no linear counter
	lda var_Linear_Counter
	and #$7F
	bpl :++								; always
:	lda var_Linear_Counter
	ora #$80							; ;; ;;;
:	sta $4008
@EndTriangleVolume:
	; Period table isn't limited to $7FF anymore
	lda var_ch_PeriodCalcHi + APU_TRI
	and #$F8
	beq @TimerOverflow3
	lda #$07
	sta var_ch_PeriodCalcHi + APU_TRI
	lda #$FF
	sta var_ch_PeriodCalcLo + APU_TRI
@TimerOverflow3:
;	lda #$08
;	sta $4009
	lda var_ch_PeriodCalcLo + APU_TRI
	sta $400A
	
	lda var_ch_Trigger + APU_TRI		;;; ;; ;
	bne :+
	lda var_ch_LengthCounter + APU_TRI
	and #%00000011
	bne @SkipTriangleKill
:	lda var_ch_LengthCounter + APU_TRI
	and #%11111000
	ora var_ch_PeriodCalcHi + APU_TRI	; ;; ;;;
	sta $400B
	jmp @SkipTriangleKill
@KillTriangle:
	lda #$00
	sta $4008
@SkipTriangleKill:

; ==============================================================================
;  Noise
; ==============================================================================
@Noise:
.if .defined(CHANNEL_CONTROL)
	lda var_Channels
	and #$08
	bne :+						; branch
	jmp @DPCM
:
.endif

	lda var_ch_Note + APU_NOI
	bne :+						; branch
	jmp @KillNoise
:

	; Calculate volume
	lda var_ch_LengthCounter + APU_NOI	;;; ;; ;
	and #$01
	beq :+
	lda var_ch_VolColumn + APU_NOI		; do not automatically kill channel when hardware envelope is enabled
	asl a
	and #$F0
	ora var_ch_Volume + APU_NOI
	tax
	lda ft_volume_table, x				; ignore tremolo
	bpl @DoneVolumeNoise				; always ; ;; ;;;
:	lda var_ch_VolColumn + APU_NOI		; Kill channel if volume column = 0
	asl a
	beq @KillNoise
	and #$F0
	sta var_Temp
	lda var_ch_Volume + APU_NOI
	beq @KillNoise
	ora var_Temp
	tax
	lda ft_volume_table, x
	sec
	sbc var_ch_TremoloResult + APU_NOI
	bpl :+
	lda #$00
:   bne :+
	lda var_ch_VolColumn + APU_NOI
	beq :+
	lda #$01
:
@DoneVolumeNoise:
	; Write to registers
	sta var_Temp		;;; ;; ;
	lda var_ch_LengthCounter + APU_NOI
	and #$03
	eor #$03
	asl a
	asl a
	asl a
	asl a
	ora var_Temp		; ;; ;;;
	sta $400C
	lda #$00
	sta $400D
	lda var_ch_DutyCurrent + APU_NOI
;	and #$01
	ror a
	ror a
	and #$80
	sta var_Temp
.if 0
.if .defined(SCALE_NOISE)
	; Divide noise period by 16
	lda var_ch_PeriodCalcLo + APU_NOI
	lsr a
	lsr a
	lsr a
	lsr a
.else
	; Limit noise period to range 0 - 15
	lda var_ch_PeriodCalcHi + APU_NOI
	bne :+
	lda var_ch_PeriodCalcLo + APU_NOI
	cmp #$10
	bcc :++
:   lda #$0F
:   eor #$0F
.endif
.else
; No limit
	lda var_ch_PeriodCalcLo + APU_NOI
	and #$0F
	eor #$0F
.endif
	ora var_Temp
	sta $400E
	lda var_ch_LengthCounter + APU_NOI	;;; ;; ;
	and #$03
	beq :+
	lda var_ch_Trigger + APU_NOI
	beq @DPCM
:	lda var_ch_LengthCounter + APU_NOI
	sta $400F							; ;; ;;;
	jmp @DPCM
@KillNoise:
	lda #$30
	sta $400C
@DPCM:

; ==============================================================================
;  DPCM
; ==============================================================================
.if .defined(USE_DPCM)
.if .defined(CHANNEL_CONTROL)
	lda var_Channels
	and #$10
	bne :+
	rts                             ; Skip DPCM
	;beq @Return
:
.endif
.if .defined(USE_ALL)		;;; ;; ;
	ldx #DPCM_OFFSET
.elseif .defined(USE_N163)
	ldx var_EffChannels
.else
	ldx #DPCM_OFFSET
.endif
	lda var_ch_DPCM_Retrig			; Retrigger
	beq @SkipRetrigger
	dec var_ch_DPCM_RetrigCntr
	bne @SkipRetrigger
	sta var_ch_DPCM_RetrigCntr
	lda #$01
	sta var_ch_Note, x
@SkipRetrigger:
	lda var_ch_DPCMDAC				; See if delta counter should be updated
	bmi @SkipDAC
	sta $4011
@SkipDAC:
	lda #$80						; store a negative value to mark that it's already updated
	sta var_ch_DPCMDAC

	lda var_ch_DPCMPhaseReset		; trigger note again if phase is reset
	bne @DPCMPhaseReset

	lda var_ch_Note, x
	beq @KillDPCM
	bmi @SkipDPCM
	lda var_ch_SamplePitch
	and #$40
	sta var_Temp
	lda var_ch_DPCM_EffPitch
	bpl :+
	lda var_ch_SamplePitch
:   ora var_Temp
	sta $4010
	lda #$80
	sta var_ch_DPCM_EffPitch

	; Setup sample bank (if used)
 .if .defined(USE_BANKSWITCH)
	lda var_ch_SampleBank
	beq :+
	jsr ft_bankswitch2
:
.endif

	; Sample position (add sample offset)
	clc
	lda var_ch_SamplePtr
	adc var_ch_DPCM_Offset
	sta $4012

	; Sample length (remove sample offset)
	lda var_ch_DPCM_Offset
	asl a
	asl a
	sta var_Temp
	sec
	lda var_ch_SampleLen
	sbc var_Temp
	sta $4013
	lda #$80
	sta var_ch_Note, x
	lda #$0F
	sta $4015
	lda #$1F
	sta $4015
	rts
@SkipDPCM:
	cmp #$FF
	beq @ReleaseDPCM
	rts
@ReleaseDPCM:
; todo
	lda #$0F
	sta $4015
	lda #$80
	sta var_ch_Note, x
	rts
@DPCMPhaseReset:
	lda #$00
	sta var_ch_DPCMPhaseReset
	lda #$01
	sta var_ch_Note, x
	rts
@KillDPCM:
	lda #$0F
	sta $4015
	lda #$80
	sta $4011
	sta var_ch_Note, x
.endif
@Return:
	rts

; Lookup tables

ft_duty_table:
.repeat 4, i
	.byte $40 * i
.endrep

ft_duty_2a03_to_vrc6:
	.byte $01, $03, $07, $03
ft_duty_vrc6_to_2a03:
	.byte $00, $00, $01, $01, $01, $01, $02, $02

; Volume table: (column volume) * (instrument volume)
ft_volume_table:
.repeat 16, xx
	.repeat 16, yy
		.if xx = 0 || yy = 0
			.byte 0
		.elseif xx * yy < 15
			.byte 1
		.else
			.byte xx * yy / 15
		.endif
	.endrep
.endrep
