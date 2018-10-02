;
; Sound effect support for FamiTracker
;
; Version 0.1
;

.segment "BSS"

ft_sq_last:			.res 2

ft_sfx_inst:		.res 1
ft_sfx_note:		.res 1
ft_sfx_chan:		.res 1
ft_sfx_duration:	.res 1

var_sfx_durations:	.res WAVE_CHANS
var_sfx_notes:		.res WAVE_CHANS

.segment "CODE"

;
; Starts a sound effect
;
; A = effect
;
ft_trigger_sfx:

	; Load from table
	asl a
	asl a
	tax
	ldy #$00
:	lda ft_sfx_table, x
	sta ft_sfx_inst, y
	inx
	iny
	cpy #$04
	bne :-
	
	;ldx ft_sfx_chan

	ldx #$01

	jsr ft_disable_channel
	
	lda ft_sfx_duration
	sta var_sfx_durations, x
	lda ft_sfx_note
	sta var_sfx_notes, x

	; Setup the effect

	clc
	lda #WAVE_CHANS
	;adc ft_sfx_chan
	adc #$01
	tax
	
	; Load instrument
	lda ft_sfx_inst
	asl a
	jsr ft_load_instrument

	; Load note
	lda ft_sfx_note
;	sta var_ch_Note + 1, x
	jsr ft_translate_freq_only

	; Turn of music channel
;	ldx ft_sfx_chan;#$00
;	jsr ft_disable_channel
	
	rts
	
;
; Updates sound effects
;
ft_play_sfx:

	ldx #$00
	; Loop through all channels
@Loop:
	lda var_sfx_durations, x
	beq @NextChan
	dec var_sfx_durations, x
	bne :+
	; Sound effect is done, reactivate music channel
	jsr ft_enable_channel
:

	txa
	pha
	clc
	adc #WAVE_CHANS
	tax
	
	; Update channel
;	lda var_sfx_notes, x
;	lda #$30
;	sta var_ch_Note, x
	jsr ft_run_instrument

	pla
	tax

@NextChan:
	inx
	cpx #$02
	bne @Loop
	
	; Update APU
	
	; Square 1
	lda var_sfx_durations
	beq :+
	lda var_ch_Volume + WAVE_CHANS
	pha
	lda var_ch_DutyCycle + WAVE_CHANS
	and #$03
	tax
	pla
	ora ft_duty_table, x
	ora #$30
	sta $4000
	lda var_ch_TimerPeriodLo
	sta $4002
	lda var_ch_TimerPeriodHi + SFX_WAVE_CHANS
	cmp ft_sq_last
	beq :+
	sta $4003
	sta ft_sq_last
:
	; Square 2

	lda var_sfx_durations + 1
	beq :+
	lda var_ch_Volume + WAVE_CHANS + 1
	pha
	lda var_ch_DutyCycle + WAVE_CHANS + 1
	and #$03
	tax
	pla
	ora ft_duty_table, x
	ora #$30
	sta $4004
	lda var_ch_TimerPeriodLo + 1
	sta $4006
	lda var_ch_TimerPeriodHi + SFX_WAVE_CHANS + 1
	cmp ft_sq_last + 1
	beq :+
	sta $4007
	sta ft_sq_last + 1
:



	lda #$20
	sta $2006
	sta $2006
	
	lda var_sfx_durations + 1
	sta $2007
	lda #$00
	sta $2007
	lda var_ch_Volume + WAVE_CHANS + 1
	sta $2007
	lda #$00
	sta $2007
	lda var_ch_TimerPeriodLo + 1
	sta $2007
	lda #$00
	sta $2007
	lda var_ch_TimerPeriodHi + SFX_WAVE_CHANS + 1
	sta $2007

	rts
	
ft_sfx_table:

	; Sound effect table: instrument, note, channel, duration
	
	.byte $01, $30, $01, $16	; 
	.byte $02, $30, $00, $16	; 
	
	.byte $0B, $3C, $00, $23	; Coin
	.byte $09, $41, $00, $31	; 1 up
	.byte $00, $00, $00, $00
	.byte $00, $00, $00, $00		
	
	; Todo: add priority
	