; Takes care of the FDS registers

ft_load_inst_extra_fds:
	lda ft_channel_type, x	;;; ;; ; non-FDS instruments do not affect wave
	cmp #CHAN_FDS
	beq :+
	ldy var_Temp
	rts
:
	txa
	pha
	ldx #$00
	tya							;;; ;; ;
	clc
	adc #$10
	sta var_Temp2				; ;; ;;;
	; Load modulation table
:
	; mod table is 32 entries
	; each entry is 3 bits long
	; data is compressed to 16 bytes
	lda (var_Temp_Pointer), y	; 5 cycles
	sta var_ch_ModTable, x		; 5 cycles
	iny							; 2 cycles
	inx							; 2 cycles
	cpy var_Temp2				; 3 cycles
	bcc :-						; 3 cycles (last iteration is 2 cycles)
	jsr ft_write_modtable		; total of 20 x 16 - 1 iterations = 319 cycles
	pla
	tax

	lda (var_Temp_Pointer), y	; Modulation delay
	iny
	sta var_ch_ModDelay
	lda (var_Temp_Pointer), y	; Modulation depth
	iny
	sta var_ch_ModInstDepth		;;; ;; ; keep state
	lda (var_Temp_Pointer), y	; Modulation freq low
	iny
	sta var_ch_ModInstRate
	lda (var_Temp_Pointer), y	; Modulation freq high
	sta var_ch_ModInstRate + 1
	iny							;;; ;; ;

	lda (var_Temp_Pointer), y	; Load wave index
	jsr ft_load_fds_wave
	ldy var_Temp
	rts							;;; ;; ;

ft_init_fds:
	lda #$00
	sta $4023
	lda #$83
	sta $4023
	lda #$FF					;;; ;; ;
	sta $408A
	lda #$80
	sta var_ch_FDSVolume
	sta var_ch_ModBias			; ;; ;;;
	rts

; Update FDS
ft_update_fds:
	lda var_PlayerFlags
	bne @Play
	lda #$80
	sta $4080
	rts
@Play:
.if .defined(USE_LINEARPITCH)		;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :+
	jsr ft_load_fds_table
	ldx #FDS_OFFSET
	jsr ft_linear_fetch_pitch
:
.endif								; ;; ;;;

	lda var_ch_Note + FDS_OFFSET
	bne :+ ; branch
	jmp @KillFDS
:
	; Calculate volume
	lda var_ch_VolColumn + FDS_OFFSET		; Kill channel if volume column = 0
	lsr a
	lsr a
	lsr a
	beq @KillFDS
	sta var_Temp2							; 4 bit vol
	lda var_ch_Volume + FDS_OFFSET			; Kill channel if volume = 0
	beq @KillFDS
	sta var_Temp							; 5 bit vol
	ldx #FDS_OFFSET
	jsr ft_multiply_volume
	sec
	sbc var_ch_TremoloResult + FDS_OFFSET
	bpl :+
@NoKill:
	lda #$00
:
	; Load volume
	ora #$80
	sta var_Temp							;;; ;; ;
	lda var_ch_FDSVolume					; check the volume envelope
	bmi :+									; envelope is disabled
	lda var_ch_Trigger + FDS_OFFSET
	beq :++									; envelope is enabled in middle of note
:	lda var_Temp
	sta $4080								; Volume
:	lda var_ch_FDSVolume
	bmi :+
	sta $4080								; enable envelope after volume init
:											; ;; ;;;

	; Load frequency
	lda var_ch_PeriodCalcHi + FDS_OFFSET
	and #$F0
	beq :+
	lda #$FF
	sta var_ch_PeriodCalcLo + FDS_OFFSET
	lda #$0F
	sta var_ch_PeriodCalcHi + FDS_OFFSET
:	lda var_ch_PeriodCalcHi + FDS_OFFSET
	sta $4083	; High
	lda var_ch_PeriodCalcLo + FDS_OFFSET
	sta $4082	; Low

	lda var_ch_Trigger + FDS_OFFSET			;;; ;; ;
	beq :+
	jsr ft_write_modtable
	lda var_ch_ModInstDepth					;;; ;; ;
	sta var_ch_ModDepth
	lda var_ch_ModRate + 1
	bmi :+
	lda var_ch_ModInstRate
	sta var_ch_ModRate
	lda var_ch_ModInstRate + 1
	sta var_ch_ModRate + 1					; ;; ;;;
:	jsr ft_check_fds_effects

	lda var_ch_ModDelayTick					; Modulation delay
	bne @TickDownDelay
;	lda var_ch_ModDepth						; Skip if modulation is disabled
;	beq @DisableMod

	lda var_ch_ModDepth						; Skip if modulation is disabled
	ora #$80
	sta $4084								; Store modulation depth

	jsr ft_check_fds_fm

@Return:
	lda var_ch_PhaseReset + FDS_OFFSET
	bne @FDSPhaseReset
	rts
@KillFDS:
	lda var_ch_FDSVolume					;;; ;; ; return if volume envelope is enabled
	bpl @NoKill								; ;; ;;;
	lda #$80
	padjmp_h	7
	sta $4080	; Make channel silent
	sta $4084
	sta $4087
	rts
	padjmp		6
@TickDownDelay:
	dec var_ch_ModDelayTick
@DisableMod:
	; Disable modulation
	lda #$80
	sta $4087
	rts
@FDSPhaseReset:
	lda #$00
	sta var_ch_PhaseReset + FDS_OFFSET
	lda #$80
	sta $4083
	lda var_ch_PeriodCalcHi + FDS_OFFSET
	sta $4083
	rts
; Load the waveform, index in A
ft_load_fds_wave:
	sta var_Temp16 + 1		;;; ;; ;
	lda #$00
	sta var_Temp16
	; Multiply by 64
	lsr var_Temp16 + 1
	ror var_Temp16
	lsr var_Temp16 + 1
	ror var_Temp16
	; Setup a pointer to the specified wave
	clc
	lda var_Wavetables
	adc var_Temp16
	sta var_Temp16
	lda var_Wavetables + 1
	adc var_Temp16 + 1
	sta var_Temp16 + 1
	; Write wave
	lda #$80
	sta $4089		; Enable wave RAM
	ldy #$3F		; optimization
:	lda (var_Temp16), y	          	; 5
	sta $4040, y					; 5
	dey								; 2
	bpl :-							; 3 = 15 cycles and 64 iterations = 960 cycles
	lda #$00
	sta $4089		; Disable wave RAM
	rts

ft_write_modtable:
	txa
	pha
	lda #$80
	sta $4087
	ldx #$00
:
	lda var_ch_ModTable, x			; 4
	pha								; 3
	and #$07						; 2
	sta $4088						; 4
	pla								; 4
	lsr a							; 2
	lsr a							; 2
	lsr a							; 2
	sta $4088						; 4
	inx								; 2
	cpx #$10						; 2
	bcc :-							; 3*16 - 1 = 543 cycles
	lda #$00
	sta $4085
	pla
	tax
	rts

ft_check_fds_effects:
	lda var_ch_ModEffWritten
	and #$01
	beq :+
	; FDS modulation depth
	lda var_ch_ModEffDepth
	sta var_ch_ModDepth
:   lda var_ch_ModEffWritten
	and #$02
	beq :+
	; FDS modulation rate high
	lda var_ch_ModEffRate + 1
	sta var_ch_ModRate + 1
:   lda var_ch_ModEffWritten
	and #$04
	beq :+
	; FDS modulation rate low
	lda var_ch_ModEffRate + 0
	sta var_ch_ModRate + 0
:
	lda #$00
	sta var_ch_ModEffWritten

	rts

ft_check_fds_fm:
	lda var_ch_ModRate + 1					;;; ;; ;
	bmi @AutoFM
	lda var_ch_ModRate						; Modulation freq
	sta $4086
	lda var_ch_ModRate + 1
	sta $4087
	rts
@AutoFM:
	; ModFreq = PeriodCalc * ModRate_Hi / ModRate_Lo + ModBias
	lda var_ch_ModRate + 1
	and #$7F
	sta var_Temp
	lda var_ch_ModRate + 0
	sta AUX
	lda var_ch_FDSCarrier + 1
	sta var_Temp16 + 1
	lda var_ch_FDSCarrier
	sta var_Temp16
	lda #$00
	sta AUX + 1
	jsr MUL

	lda EXT
	beq :+
	lda #$FF
	sta ACC
	sta ACC + 1
:	jsr DIV

	lda var_ch_ModBias
	eor #$80
	bpl :+
	dec ACC + 1
:	clc
	adc ACC

	; if (ModFreq > 0xFFF) ModFreq = 0xFFF;
	clc
	lda ACC + 1
	cmp #$10
	bcc :+
	lda #$0F
	sta ACC + 1
	lda #$FF
	sta ACC + 0

:	clc
	lda ACC + 0
	sta $4086
	lda ACC + 1
	sta $4087
	rts
