;
; Namco 163 expansion sound
;

; Load N163 instrument
ft_load_inst_extra_n163:
	lda ft_channel_type, x	;;; ;; ; non-N163 instruments do not affect wave
	cmp #CHAN_N163
	beq :+
	tya
	clc
	adc #$04
	tay
	bne @DoneParams ; always
:	lda var_ch_WaveLen - N163_OFFSET, x
	and #$80
	ora (var_Temp_Pointer), y
	sta var_ch_WaveLen - N163_OFFSET, x
	bmi :+
	iny
	lda (var_Temp_Pointer), y
	sta var_ch_WavePos - N163_OFFSET, x
	sta var_ch_WavePosOld - N163_OFFSET, x
	iny
	jmp :++
:	iny
	lda (var_Temp_Pointer), y
	sta var_ch_WavePosOld - N163_OFFSET, x
	iny							; ;; ;;;
:
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp_Pointer), y
	adc ft_music_addr
	sta var_ch_WavePtrLo - N163_OFFSET, x
	iny
	lda (var_Temp_Pointer), y
	adc ft_music_addr + 1
	sta var_ch_WavePtrHi - N163_OFFSET, x
	iny
.else
	lda (var_Temp_Pointer), y
	sta var_ch_WavePtrLo - N163_OFFSET, x
	iny
	lda (var_Temp_Pointer), y
	sta var_ch_WavePtrHi - N163_OFFSET, x
	iny
.endif
@DoneParams:
    ; check if non-N163 type
	lda ft_channel_type, x
	cmp #CHAN_N163
	bne :++
    ; check if N163 instrument is about to change
	lda var_NamcoInstrument - N163_OFFSET, x
	cmp var_Temp2
	beq :+
	lda #$00             ; reset wave
	sta var_ch_DutyDefault, x
	sta var_ch_DutyCurrent, x
	lda var_Temp2
	; Load N163 wave
;    jsr ft_n163_load_wave
:   sta var_NamcoInstrument - N163_OFFSET, x
	lda ft_channel_type, x	;;; ;; ;
	cmp #CHAN_N163
	bne :+					; ;; ;;;
	jsr ft_n163_load_wave2
:	ldy var_Temp
	rts

ft_init_n163:
	; Enable sound, copied from PPMCK and verified on a real Namco cart
	; no sound without this!
.if .defined(USE_S5B)		;;; ;; ;
	lda #$0E
	sta $C000
.endif						; ;; ;;;
	lda #$20
	sta $E000
	; Enable all channels
	lda #$7F
	sta $F800
	lda #$70
	sta $4800
	; Clear wave ram
	lda #$80
	sta $F800
	ldx #$7E
	lda #$00
:   sta $4800
	dex
	bne :-
	ldx #$07
:	lda #$01				;;; ;; ;
	sta var_ch_WaveLen, x
	dex
	bpl :-

	rts

; Update N163 channels
ft_update_n163:
.if .defined(USE_S5B)		;;; ;; ;
	lda #$0E
	sta $C000
.endif						; ;; ;;;
	; Check player flag
	lda var_PlayerFlags
	bne @Play
	; Kill all channels
	lda #$C0
	sta $F800
	tax
	lda #$00
:   sta $4800
	dex
	bne :-
	rts
@Play:
.if .defined(USE_LINEARPITCH)		;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :++
	jsr ft_load_n163_table
	lda var_NamcoChannels
	sta var_Temp3
	ldx #N163_OFFSET
:	jsr ft_linear_fetch_pitch
	dec var_Temp3
	bne :-
:
.endif								; ;; ;;;
	; x = channel
	ldx #$00
@ChannelLoop:
	; Begin
	; Set address
	lda #$00
	jsr @LoadAddr

	lda var_ch_Note + N163_OFFSET, x
	beq @KillChannel  ; out of range

	; Get volume
	lda var_ch_VolColumn + N163_OFFSET, x		; Kill channel if volume column = 0
	asl a
	beq @KillChannel
	and #$F0
	sta var_Temp
	lda var_ch_Volume + N163_OFFSET, x
	beq @KillChannel
	ora var_Temp
	tay
	lda ft_volume_table, y
	sec
	sbc var_ch_TremoloResult + N163_OFFSET, x
	bpl :+
	lda #$00
:   bne :+
	lda var_ch_VolColumn + N163_OFFSET, x
	beq :+
	lda #$01
:

	sta var_Temp

	; Load frequency regs
	jsr @LoadPeriod

	; Write regs
	lda var_Temp2
	sta $4800 	; Low part of freq

	lda #$02
	jsr @LoadAddr

	lda var_Temp3
	sta $4800 	; Middle part of freq

	lda #$04
	jsr @LoadAddr

	lda var_Temp4
	and #$03
	sta var_Temp4

	lda var_ch_WaveLen, x
	sec
	sbc #$01
	lsr a
	eor #$FF
	asl a
	asl a
	ora var_Temp4
	sta $4800 	; Wave size, High part of frequency

	lda #$06
	jsr @LoadAddr

	lda var_ch_WavePos, x
	sta $4800 	; Wave position

	lda var_NamcoChannelsReg
	ora var_Temp
	sta $4800 	; Volume

	jmp @SkipChannel

@KillChannel:
	ldy #$07
:   sta $4800
	dey
	bne :-
	lda var_NamcoChannelsReg
	sta $4800

@SkipChannel:
	; End
	lda var_ch_PhaseReset + N163_OFFSET, x
	bne @N163PhaseReset
	inx
	cpx var_NamcoChannels
	beq :+
	jmp @ChannelLoop
:	rts

@N163PhaseReset:
	dec var_ch_PhaseReset + N163_OFFSET, x

	lda #$01
	jsr @LoadAddr					; low phase
	lda #$00
	sta $4800

	lda #$03
	jsr @LoadAddr					; mid phase
	lda #$00
	sta $4800

	lda #$05
	jsr @LoadAddr					; hi phase
	lda #$00
	sta $4800
	rts

@LoadAddr:                    ; Load N163 RAM address
	clc
	adc ft_n163_chan_addr, x
	ora #$80	              ; Auto increment
	sta $F800
	rts

@LoadPeriod:
	lda #$00
	sta var_Temp4
	lda var_ch_PeriodCalcHi + N163_OFFSET, x
	sta var_Temp3
	lda var_ch_PeriodCalcLo + N163_OFFSET, x

	asl a
	rol var_Temp3
	rol var_Temp4
	asl a
	rol var_Temp3
	rol var_Temp4
	sta var_Temp2		;;; ;; ;

.if 0
	; Compensate for shorter wave lengths
	lda var_ch_WaveLen, x
	cmp #$10
	beq :+
	lsr var_Temp4
	ror var_Temp3
	ror var_Temp2
	cmp #$08
	beq :+
	lsr var_Temp4
	ror var_Temp3
	ror var_Temp2
	cmp #$04
	beq :+
	lsr var_Temp4
	ror var_Temp3
	ror var_Temp2
	cmp #$02
	beq :+
	lsr var_Temp4
	ror var_Temp3
	ror var_Temp2
:
.endif
	rts

ft_n163_load_wave2:
	lda var_ch_InstType, x				;;; ;; ;
	cmp #CHAN_N163
	beq :+
	rts
:										; ;; ;;;
.if .defined(USE_S5B)		;;; ;; ;
	lda #$0E
	sta $C000
.endif						; ;; ;;;

	tya
	pha

	; Get wave pack pointer
	lda var_ch_WavePtrLo - N163_OFFSET, x
	sta var_Temp_Pointer2
	lda var_ch_WavePtrHi - N163_OFFSET, x
	sta var_Temp_Pointer2 + 1

	; Get number of waves
	ldy #$00
	lda (var_Temp_Pointer2), y
	sta var_Temp3

	; Setup wave RAM
	lda var_ch_WavePos - N163_OFFSET, x
	lsr a
	ora #$80
	sta $F800

	; Get wave index
	lda var_ch_DutyCurrent, x
	beq @EndMul		;;; ;; ; Multiply wave index with wave len
.if .defined(USE_MMC5) && .defined(USE_MMC5_MULTIPLIER)
	sta $5205
	lda	var_ch_WaveLen - N163_OFFSET, x
	and #$7F
	sta $5206
	clc
	lda $5205
	adc var_Temp_Pointer2
	sta var_Temp_Pointer2
	lda $5206
	adc var_Temp_Pointer2 + 1
	sta var_Temp_Pointer2 + 1
.else
	sta var_Temp3
	lda	var_ch_WaveLen - N163_OFFSET, x
	and #$7F
	tay
:   tya
	clc
	adc var_Temp_Pointer2
	sta var_Temp_Pointer2
	bcc :+
	inc var_Temp_Pointer2 + 1
:	dec var_Temp3
	bne :--
.endif
@EndMul:		; ;; ;;;

	txa          ; Save X
	pha
	lda var_ch_WaveLen - N163_OFFSET, x
	and #$7F		;;; ;; ;
	tax

	; Load wave
	ldy #$00		;;; ;; ;
:	lda (var_Temp_Pointer2), y
	sta $4800
	iny
	dex
	bne :-

	pla     ; Restore x & y
	tax
	pla
	tay

	rts

ft_n163_chan_addr:
.repeat 8, i
	.byte $78 - $8 * i
.endrep
