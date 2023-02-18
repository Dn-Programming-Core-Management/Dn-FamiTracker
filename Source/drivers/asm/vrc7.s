; VRC7 commands
;  0 = halt
;  1 = trigger
;  80 = update

VRC7_HALT       = $00
VRC7_TRIGGER    = $01
VRC7_HOLD_NOTE  = $80

ft_load_instrument_vrc7:
	; Read VRC7 instrument
	ldy #$01									;;; ;; ; skip inst type
	lda (var_Temp_Pointer), y		            ; Load patch number
	sta var_ch_DutyCurrent, x					;;; ;; ; renamed
	sta var_ch_DutyDefault, x
	bne :+							            ; Skip custom settings if patch > 0

	; Store path to custom patch settings
	clc
	lda var_Temp_Pointer
	adc #$02									;;; ;; ; skip one
	sta var_ch_vrc7_CustomLo - VRC7_OFFSET, x
	lda var_Temp_Pointer + 1
	adc #$00
	sta var_ch_vrc7_CustomHi - VRC7_OFFSET, x

:	ldy var_Temp
	rts

ft_init_vrc7:
	lda #$00
	sta var_ch_vrc7_PatchFlag
	tax
:   stx $9010
	jsr ft_vrc7_delay2
	sta $9030
	jsr ft_vrc7_delay
	inx
	cpx #$3F
	bne :-
	ldx #$05
	lda #$FF
:	sta var_ch_vrc7_EffPatch, x
	dex
	bpl :-
	rts

ft_translate_note_vrc7:
	; Calculate Fnum & Bnum
	; Input: A = note + 1
	; Result: A = Fnum index, ACC = Bnum
:	cmp #12
	bcc :+ ; sec
	sbc #12
	inc ACC
	bne :- ; always
:	rts

.if .defined(USE_LINEARPITCH)		;;; ;; ;
ft_vrc7_linear_fetch_pitch:
	jsr ft_linear_prescale
	lda #$00
	sta ACC
	lda var_ch_PeriodCalcHi, x
	jsr ft_translate_note_vrc7
	tay
	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_OFFSET, x
	lda ft_note_table_vrc7_l, y
	sta var_ch_PeriodCalcLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_PeriodCalcHi, x

	lda var_Temp
	beq :+

	iny
	sec
	lda ft_note_table_vrc7_l, y
	sbc var_ch_PeriodCalcLo, x
	sta var_Temp16
	lda ft_note_table_vrc7_h, y
	sbc var_ch_PeriodCalcHi, x
	sta var_Temp16 + 1

	jsr ft_linear__final
	dex ;
:	lsr var_ch_PeriodCalcHi, x
	ror var_ch_PeriodCalcLo, x
	lsr var_ch_PeriodCalcHi, x
	ror var_ch_PeriodCalcLo, x
	rts
.endif

ft_clear_vrc7:
	clc
	txa
	adc #$20	; $20: Clear channel
	sta $9010
	lda var_ch_vrc7_FnumHi, x
	ora var_ch_vrc7_Bnum, x
	sta $9030
	jsr ft_vrc7_delay
	rts

; Update all VRC7 channel registers
;
ft_update_vrc7:

	lda var_PlayerFlags
	bne @Play
	; Close all channels
	ldx #$06
:	txa
	clc
	adc #$1F
	sta $9010
	jsr ft_vrc7_delay2
	lda #$00
	sta $9030
	jsr ft_vrc7_delay
	dex
	bne :-
	rts
@Play:
.if .defined(USE_LINEARPITCH)		;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :++
	ldx #VRC7_OFFSET
:	jsr ft_vrc7_linear_fetch_pitch
	inx
	cpx #(VRC7_OFFSET + CH_COUNT_VRC7)
	bne :-
:
.endif								; ;; ;;;
	ldx #$00					; x = channel
@LoopChannels:

	; note off is checked in player.s
	; See if retrigger is needed
	lda var_ch_vrc7_Command, x
	cmp #VRC7_TRIGGER
	bne @UpdateChannel

	; Clear channel, this also serves as a retrigger
	jsr ft_clear_vrc7

	; Remove trigger command
	lda #VRC7_HOLD_NOTE
	sta var_ch_vrc7_Command, x

@UpdateChannel:
	; Load and cache period if there is an active note
	lda var_ch_Note + VRC7_OFFSET, x
	beq @SkipPeriod
	lda var_ch_PeriodCalcLo + VRC7_OFFSET, x
	sta var_ch_vrc7_FnumLo, x
	lda var_ch_PeriodCalcHi + VRC7_OFFSET, x
	and #$07
	sta var_ch_vrc7_FnumHi, x
@SkipPeriod:

	clc
	txa
	adc #$10	; $10: Low part of Fnum
	sta $9010
	jsr ft_vrc7_delay2
	lda var_ch_vrc7_FnumLo, x
	sta $9030
	jsr ft_vrc7_delay

	; Note on or off
	lda #$00
	sta var_Temp2
	; Skip if halt
	lda var_ch_vrc7_Command, x
	beq :+

	; Check release
	lda var_ch_State + VRC7_OFFSET, x
	and #STATE_RELEASE
	tay
	lda ft_vrc7_cmd, y
	sta var_Temp2

:	clc
	txa
	adc #$30	; $30: Patch & Volume
	sta $9010

	lda var_ch_vrc7_EffPatch, x		;;; ;; ;
	cmp #$FF
	beq :+
	sta var_ch_DutyCurrent + VRC7_OFFSET, x
	lda #$FF
	sta var_ch_vrc7_EffPatch, x
:									; ;; ;;;

	lda var_ch_VolColumn + VRC7_OFFSET, x
	lsr a
	lsr a
	lsr a
	sec
	sbc var_ch_TremoloResult + VRC7_OFFSET, x
	bpl :+
	lda #$00
:	eor #$0F
	ora var_ch_DutyCurrent + VRC7_OFFSET, x
	sta $9030
	jsr ft_vrc7_delay

	clc
	txa
	adc #$20	; $20: High part of Fnum, Bnum, Note on & sustain on
	sta $9010
	lda var_ch_vrc7_Bnum, x
	asl a
	ora var_ch_vrc7_FnumHi, x
	ora var_Temp2
	sta $9030
	jsr ft_vrc7_delay


@NextChan:
	inx
	cpx #$06
	beq :+
	jmp @LoopChannels
:
@UpdatePatch:		;;; ;; ;
	ldx #$07
:	asl var_ch_vrc7_PatchFlag
	bcc :+
	stx $9010
	jsr ft_vrc7_delay2
	lda var_ch_vrc7_Write, x
	sta $9030
	jsr ft_vrc7_delay
:	dex
	bpl :--
	rts

; Used to adjust Bnum when portamento is used
;
ft_vrc7_adjust_octave:

	; Get octave
	lda #$00		;;; ;; ;
	sta ACC
	lda var_ch_vrc7_ActiveNote - VRC7_OFFSET, x
	jsr ft_translate_note_vrc7		; ;; ;;;

	lda	ACC					; if new octave > old octave
	cmp var_ch_vrc7_OldOctave
	bcs :+
	; Old octave > new octave, shift down portamento frequency
	lda var_ch_vrc7_OldOctave
	sta var_ch_vrc7_Bnum - VRC7_OFFSET, x
	sec
	sbc ACC
	jmp @ShiftFreq2
:	lda	var_ch_vrc7_OldOctave	; if old octave > new octave
	cmp ACC
	bcc :+
	rts
	; New octave > old octave, shift down old frequency
:	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_OFFSET, x
	sec
	sbc var_ch_vrc7_OldOctave
;	jmp @ShiftFreq

@ShiftFreq:
	sty var_Temp
	tay
:	lsr var_ch_TimerPeriodHi, x
	ror var_ch_TimerPeriodLo, x
	dey
	bne :-
	ldy var_Temp
	rts

@ShiftFreq2:
	sty var_Temp
	tay
:	lsr var_ch_PortaToHi, x
	ror var_ch_PortaToLo, x
	dey
	bne :-
	ldy var_Temp
	rts

; Called when a new note is found from pattern reader
ft_vrc7_trigger:
	jsr ft_get_hold_clear		;;; ;; ;
	beq :+
	rts
:	lda #$00
	sta var_ch_State, x

	lda var_ch_DutyCurrent, x
	bne @SkipCustomPatch

	lda var_ch_vrc7_CustomLo - VRC7_OFFSET, x
	sta var_CustomPatchPtr
	lda var_ch_vrc7_CustomHi - VRC7_OFFSET, x
	sta var_CustomPatchPtr + 1
	jsr ft_load_vrc7_custom_patch
@SkipCustomPatch:
	lda var_ch_State, x		;;; ;; ;
	and #STATE_RELEASE
	bne :++
	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne :+
	lda var_ch_vrc7_Command - VRC7_OFFSET, x
	bne :++
:	lda #VRC7_TRIGGER							; Trigger VRC7 channel
	sta var_ch_vrc7_Command - VRC7_OFFSET, x
	; Adjust Fnum if portamento is enabled
:	jsr ft_set_trigger		;;; ;; ;
	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne @Return
	; Load portamento
	lda var_ch_Note, x
	beq @Return
	lda var_ch_vrc7_OldOctave
	bmi @Return
	jsr ft_vrc7_adjust_octave
@Return:
	rts

ft_vrc7_get_freq:

	tya
	pha

	lda var_ch_vrc7_Command - VRC7_OFFSET, x
	cmp #VRC7_HALT
	bne :+
	; Clear old frequency if channel was halted
	lda #$00
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x

:	lda var_ch_vrc7_Bnum - VRC7_OFFSET, x
	sta var_ch_vrc7_OldOctave

	; Retrigger channel
	lda #$00		;;; ;; ;
	sta ACC		; ;; ;;;
	lda var_ch_vrc7_ActiveNote - VRC7_OFFSET, x
	jsr ft_translate_note_vrc7
	tay

	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne @NoPorta
	lda ft_note_table_vrc7_l, y
	sta var_ch_PortaToLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_PortaToHi, x

	; Check if previous note was silent, move this frequency directly to it
	lda var_ch_TimerPeriodLo, x
	ora var_ch_TimerPeriodHi, x
	bne :+

	lda var_ch_PortaToLo, x
	sta var_ch_TimerPeriodLo, x
	lda var_ch_PortaToHi, x
	sta var_ch_TimerPeriodHi, x

	lda #$80				; Indicate new note (no previous)
	sta var_ch_vrc7_OldOctave

	jmp :+

@NoPorta:
	lda ft_note_table_vrc7_l, y
	sta var_ch_TimerPeriodLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_TimerPeriodHi, x

:	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_OFFSET, x

	pla
	tay
	jmp ft_set_trigger		;;; ;; ;

ft_vrc7_get_freq_only:
	tya
	pha

	; Retrigger channel
	lda #$00		;;; ;; ;
	sta ACC		; ;; ;;;
	lda var_ch_vrc7_ActiveNote - VRC7_OFFSET, x
	jsr ft_translate_note_vrc7
	tay

	lda ft_note_table_vrc7_l, y
	sta var_ch_TimerPeriodLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_TimerPeriodHi, x

	lda var_ch_vrc7_Bnum - VRC7_OFFSET, x
	sta var_ch_vrc7_OldOctave

	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_OFFSET, x

	jsr ft_set_trigger		;;; ;; ;

	pla
	tay

	rts

; Setup note slides
;
ft_vrc7_load_slide:

	lda var_ch_TimerPeriodLo, x
	pha
	lda var_ch_TimerPeriodHi, x
	pha

	; Load note
	lda var_ch_EffParam, x			; Store speed
	and #$0F						; Get note
	sta var_Temp					; Store note in temp

	lda var_ch_Effect, x
	cmp #EFF_SLIDE_UP_LOAD
	beq :+
	lda var_ch_Note, x
	sec
	sbc var_Temp
	jmp :++
:	lda var_ch_Note, x
	clc
	adc var_Temp
:	sta var_ch_Note, x

	sta var_ch_vrc7_ActiveNote - VRC7_OFFSET, x		;;; ;; ;
	dec var_ch_vrc7_ActiveNote - VRC7_OFFSET, x
	jsr ft_vrc7_get_freq_only

	lda var_ch_TimerPeriodLo, x
	sta var_ch_PortaToLo, x
	lda var_ch_TimerPeriodHi, x
	sta var_ch_PortaToHi, x

	; Store speed
	lda var_ch_EffParam, x
	lsr a
	lsr a
	lsr a
	ora #$01
	sta var_ch_EffParam, x

	; Load old period
	pla
	sta var_ch_TimerPeriodHi, x
	pla
	sta var_ch_TimerPeriodLo, x

	; change mode to sliding
	clc
	lda var_ch_Effect, x
	cmp #EFF_SLIDE_UP_LOAD
	bne :+
	lda #EFF_SLIDE_DOWN
	sta var_ch_Effect, x
	jmp ft_vrc7_adjust_octave
:	lda #EFF_SLIDE_UP
	sta var_ch_Effect, x
	jmp ft_vrc7_adjust_octave

; Load VRC7 custom patch registers
ft_load_vrc7_custom_patch:
	tya
	pha
	ldy #$00
:	lda (var_CustomPatchPtr), y		            ; Load register
	sty $9010						            ; Register index
	jsr ft_vrc7_delay2
	sta $9030						            ; Store the setting
	jsr ft_vrc7_delay
	iny
	cpy #$08
	bne :-
	pla
	tay
	rts

ft_vrc7_delay:
	pha
	lda	#$01
:	asl	a
	bcc	:-
	pla
ft_vrc7_delay2:		;;; ;; ;
	rts

ft_vrc7_cmd:
	.byte $30, $20
