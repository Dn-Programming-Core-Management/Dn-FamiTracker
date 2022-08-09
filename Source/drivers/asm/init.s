;
; ft_sound_init
;
; Initializes the player and song number
; a = song number
; x = ntsc/pal
;
ft_music_init:
	asl a
	jsr ft_load_song
	; Kill APU registers
	lda #$00
	ldx #$0B
@LoadRegs:
	sta $4000, x
	dex
	bne @LoadRegs
	ldx #$06
@LoadRegs2:
	sta $400D, x
	dex
	bne @LoadRegs2
	lda #$30		; noise is special
	sta $400C
	lda #$0F
	sta $4015		; APU control
	lda #$08
	sta $4001
	sta $4005
	lda #$80		;;; ;; ;
	sta $4017
	lda #$00
	sta $4017		; ;; ;;; Disable frame IRQs

	lda #$FF		;;; ;; ; Reset triangle linear counter
	sta var_Linear_Counter
.if .defined(CHANNEL_CONTROL)
	sta var_Channels	; Enable all channels
.endif

	sta var_ch_DPCM_EffPitch
	sta var_ch_DPCMDAC

	; Reset some variables for the wave channels
	ldx #$00							;;; ;; ;
:

	cpx #(CH_COUNT_2A03 + CH_COUNT_MMC5)
	bpl :+
	lda #$08
	sta var_ch_LengthCounter, x			;;; ;; ;
:	lda #$00
	sta var_ch_NoteRelease, x			;;; ;; ;
	sta var_ch_Transpose, x				;;; ;; ;
	sta var_ch_NoteCut, x
	sta var_ch_Effect, x
	sta var_ch_EffParam, x
	sta var_ch_PortaToLo, x
	sta var_ch_PortaToHi, x
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
	sta var_ch_Trigger, x				;;; ;; ;
	inx
	cpx #WAVE_CHANS
	bne :--								; ;; ;;;

.if .defined(USE_OLDVIBRATO)		;;; ;; ;
	lda var_SongFlags
	and #FLAG_OLDVIBRATO
	beq :+
	lda #48
:
.endif
	ldx #$00
:	sta var_ch_VibratoPos, x
	inx
	cpx #WAVE_CHANS
	bne :-
.if .defined(USE_OLDVIBRATO)		;;; ;; ;
	lda #$00
.endif

	; DPCM
.if .defined(USE_DPCM)
	sta var_ch_NoteCut + DPCM_OFFSET
	sta var_ch_NoteRelease + DPCM_OFFSET	;;; ;; ;
.if .defined(USE_ALL)
	ldx #EFF_CHANS
.elseif .defined(USE_N163)
	ldx var_EffChannels
.else
	ldx #EFF_CHANS
.endif
	lda #$80
	sta var_ch_Note, x
.endif

.if .defined(USE_VRC6)
	lda #$00
	sta $9003
.endif

.if .defined(USE_MMC5)
	lda #$03
	sta $5015		; Enable channels
.endif

.if .defined(USE_N163)
	jsr ft_init_n163
.endif

.if .defined(USE_VRC7)
	jsr ft_init_vrc7
.endif

.if .defined(USE_FDS)
	jsr ft_init_fds
.endif

.if .defined(USE_S5B)		;;; ;; ;
	jsr ft_init_s5b
.endif						; ;; ;;;

	rts

;
; Prepare the player for a song
;
; NSF music data header:
;
; - Song list, 2 bytes
; - Instrument list, 2 bytes
; - DPCM instrument list, 2 bytes
; - DPCM sample list, 2 bytes
; - Flags, 1 byte
; - Pointer to wave tables, 2 bytes, if FDS is enabled
; - NTSC speed divider
; - PAL speed divider
;
ft_load_song:
	pha
	; Get the header
	lda ft_music_addr
	sta var_Temp_Pointer
	lda ft_music_addr + 1
	sta var_Temp_Pointer + 1

	; Read the header and store in RAM
	ldy #$00
@LoadAddresses:
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp_Pointer), y
	adc ft_music_addr
	sta var_Song_list, y
	iny
	lda (var_Temp_Pointer), y			; Song list offset, high addr
	adc ft_music_addr + 1
	sta var_Song_list, y
.else
	lda (var_Temp_Pointer), y
	sta var_Song_list, y
	iny
	lda (var_Temp_Pointer), y			; Song list offset, high addr
	sta var_Song_list, y
.endif
	iny
	cpy #$0A							;;; ;; ; 5 items, now including groove table
	bne @LoadAddresses

	lda (var_Temp_Pointer), y			; Flags, 1 byte
	sta var_SongFlags
	iny

.if .defined(USE_FDS)
	; Load FDS wave table pointer
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp_Pointer), y
	adc ft_music_addr
	sta var_Wavetables
	iny
	lda (var_Temp_Pointer), y
	adc ft_music_addr + 1
	sta var_Wavetables + 1
.else
	lda (var_Temp_Pointer), y
	sta var_Wavetables
	iny
	lda (var_Temp_Pointer), y
	sta var_Wavetables + 1
.endif
	iny
.endif

.if .defined(PAL_PERIOD_TABLE)			;;; ;; ;
	cpx #$01							; PAL / NTSC flag
	beq @LoadPAL
.endif									; ;; ;;;
.if .defined(NTSC_PERIOD_TABLE)
	; Load NTSC speed divider and frequency table
	lda (var_Temp_Pointer), y
	iny
	sta var_Tempo_Dec
	lda (var_Temp_Pointer), y
	iny
	sta var_Tempo_Dec + 1
	jsr ft_load_ntsc_table		;;; ;; ;
.if .defined(USE_N163)
	iny
	iny
.endif
.endif
.if .defined(PAL_PERIOD_TABLE)
	jmp @LoadDone
@LoadPAL:
	; Load PAL speed divider and frequency table
	iny
	iny
	lda (var_Temp_Pointer), y
	iny
	sta var_Tempo_Dec
	lda (var_Temp_Pointer), y
	iny
	sta var_Tempo_Dec + 1
	lda var_PlayerFlags		;;; ;; ;
	ora #FLAG_USEPAL
	sta var_PlayerFlags
	jsr ft_load_pal_table		; ;; ;;;
.endif
@LoadDone:
	clc
.if .defined(USE_N163)
	; N163 channel count
	lda (var_Temp_Pointer), y
	iny
	sta var_NamcoChannels
	adc #(CH_COUNT_2A03 + CH_COUNT_VRC6 + CH_COUNT_MMC5 + CH_COUNT_FDS + CH_COUNT_VRC7 + CH_COUNT_S5B)
.else
	lda #(CH_COUNT_2A03 + CH_COUNT_VRC6 + CH_COUNT_MMC5 + CH_COUNT_FDS + CH_COUNT_VRC7 + CH_COUNT_S5B)
.endif
	sta var_EffChannels
	adc #$01
	sta var_AllChannels

.if .defined(USE_N163)
	ldx var_NamcoChannels
	dex
	txa
	asl a
	asl a
	asl a
	asl a
	sta var_NamcoChannelsReg
.endif

	pla
	tay
	; Load the song
	jsr ft_load_track

	; Clear variables to zero
	; Important!
	ldx #$01
	stx var_PlayerFlags				; Player flags, bit 0 = playing
	dex
@ClearChannels2:					; This clears the first four channels
	lda #$7F
	sta var_ch_VolColumn, x
	sta var_ch_VolDefault, x		;;; ;; ;
	lda #$80
	sta var_ch_FinePitch, x
	sta var_ch_VolSlideTarget, x	;; ;; !!
	lda #$00
	;
	;lda #$00
	sta var_ch_VibratoSpeed, x
	sta var_ch_TremoloSpeed, x
	sta var_ch_Effect, x
	sta var_ch_VolSlide, x
	sta var_ch_NoteDelay, x
	sta var_ch_ArpeggioCycle, x
	sta var_ch_Harmonic, x
	inc var_ch_Harmonic, x			; default value is 1
	;
	sta var_ch_Note, x
	inx

	cpx #EFF_CHANS		;;; ;; ;
	bne @ClearChannels2

	ldx #$FF
	stx var_ch_PrevFreqHigh			; Set prev freq to FF for Sq1 & 2
	stx var_ch_PrevFreqHigh + 1

.if .defined(USE_DPCM)
	lda #$00
	sta var_ch_DPCM_Offset
.endif
.if .defined(USE_MMC5)
	stx var_ch_PrevFreqHighMMC5
	stx var_ch_PrevFreqHighMMC5 + 1
.endif

	inx								; Jump to the first frame
	stx var_Current_Frame
	jsr ft_load_frame

	jsr ft_calculate_speed
	;jsr ft_restore_speed

	lda #$00
	sta var_Tempo_Accum
	sta var_Tempo_Accum + 1

	rts

;
; Load the track number in A
;
; Track headers:
;
;	- Frame list address, 2 bytes
;	- Number of frames, 1 byte
;	- Pattern length, 1 byte
;	- Speed, 1 byte
;	- Tempo, 1 byte
;
ft_load_track:
	; Load track header address
	lda var_Song_list
	sta var_Temp16
	lda var_Song_list + 1
	sta var_Temp16 + 1

	; Get the real address, song number * 2 will be in Y here
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

	; Read header
	lda #$00
	tay
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp_Pointer), y			; Frame offset, low addr
	adc ft_music_addr
	sta var_Frame_List
	iny
	lda (var_Temp_Pointer), y			; Frame offset, high addr
	adc ft_music_addr + 1
	sta var_Frame_List + 1
.else
	lda (var_Temp_Pointer), y			; Frame offset, low addr
	sta var_Frame_List
	iny
	lda (var_Temp_Pointer), y			; Frame offset, high addr
	sta var_Frame_List + 1
.endif
	iny ; Y == 02
@ReadLoop:
	lda (var_Temp_Pointer), y			; Frame count
	sta var_Frame_Count - 2, y
	iny
	cpy #$08							;;; ;; ; Groove
	bne @ReadLoop

	rts

;
; Load the frame in A for all channels
;
ft_load_frame:
.if .defined(USE_BANKSWITCH)
	pha						; Frame bank
	lda var_InitialBank
	beq :+
	jsr ft_bankswitch
:	pla
.endif

	; Get the entry in the frame list
	asl a					; Multiply by two
	clc						; And add the frame list addr to get
	adc var_Frame_List		; the pattern list addr
	sta var_Temp16
	lda #$00
	tay
	tax
	adc var_Frame_List + 1
	sta var_Temp16 + 1
	lda var_Current_Frame	;;; ;; ;
	bpl :+
	inc var_Temp16 + 1
:							; ;; ;;;
	; Get the entry in the pattern list
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
	; Iterate through the channels, x = channel
	ldy #$00							; Y = address
	stx var_Pattern_Pos
@LoadPatternAddr:
	CH_LOOP_START @LoadPatternAddrEpilog
.if .defined(RELOCATE_MUSIC)
	clc
	lda (var_Temp_Pointer), y			; Load the pattern address for the channel
	adc ft_music_addr
	sta var_ch_PatternAddrLo, x
	iny
	lda (var_Temp_Pointer), y			; Pattern address, high byte
	adc ft_music_addr + 1
	sta var_ch_PatternAddrHi, x
	iny
.else
	lda (var_Temp_Pointer), y			; Load the pattern address for the channel
	sta var_ch_PatternAddrLo, x
	iny
	lda (var_Temp_Pointer), y			; Pattern address, high byte
	sta var_ch_PatternAddrHi, x
	iny
.endif
	lda #$00
	sta var_ch_NoteDelay, x
	sta var_ch_Delay, x
;	sta var_ch_LoopCounter, x
	lda #$FF
	sta var_ch_DefaultDelay, x
@LoadPatternAddrEpilog:
	CH_LOOP_END @LoadPatternAddr
; Bankswitch values
.if .defined(USE_BANKSWITCH)
	lda var_SongFlags					; Check bankswitch flag
	and #FLAG_BANKSWITCH
	beq @SkipBankValues					; Skip if no bankswitch info is stored
	ldx #$00
@LoadBankValues:
	CH_LOOP_START @LoadBankValuesEpilog
	lda (var_Temp_Pointer), y			; Pattern bank number
	sta var_ch_Bank, x
	iny
@LoadBankValuesEpilog:
	CH_LOOP_END @LoadBankValues
@SkipBankValues:
.endif

	lda #$00
	sta var_Jump
	sta var_Skip

.if .defined(ENABLE_ROW_SKIP)
	lda var_SkipTo
	bne ft_SkipToRow
	rts
;
; Skip to a certain row, this is NOT recommended in songs when CPU time is critical!!
;
;;; ;; ; various optimizations
ft_SkipToRow:
	sta var_Pattern_Pos
	ldx #$00							; x = channel
@ChannelLoop:
	CH_LOOP_START @ChannelLoopEpilog
.if .defined(USE_BANKSWITCH)			;;; ;; ; perform bankswitching
	lda var_ch_Bank, x
	beq :+
	jsr ft_bankswitch
:
.endif									; ;; ;;;
	lda #$FF
	sta var_Temp3
	lda var_Pattern_Pos
	sta var_Temp2						; Restore row count
	lda #$00
	sta var_ch_NoteDelay, x
	tay
	lda var_ch_PatternAddrLo, x
	sta var_Temp_Pattern
	lda var_ch_PatternAddrHi, x
	sta var_Temp_Pattern + 1

@RowLoop:
	lda var_ch_NoteDelay, x				; First check if in the middle of a row delay
	beq @NoRowDelay
	lda var_Temp2
	sec
	sbc var_ch_NoteDelay, x
	bcc :+
	pha
	lda #$00
	sta var_ch_NoteDelay, x
	pla
	sta var_Temp2
	bne @NoRowDelay
	beq @Finished
:	lda var_ch_NoteDelay, x
	sec
	sbc var_Temp2
	sta var_ch_NoteDelay, x
	lda #$00
	sta var_Temp2
	jmp @Finished

@EffectDispatch:		;;; ;; ;
	jsr @Effect
@NoRowDelay:
	; Read a row
	lda (var_Temp_Pattern), y
	bmi @EffectDispatch

	lda var_ch_DefaultDelay, x
	cmp #$FF
	bne :+
	iny
	lda (var_Temp_Pattern), y
:	iny
	sta var_ch_NoteDelay, x				; Store default delay

	; Save the new address
	tya
	bpl :+								; defer as many additions as possible
	clc
	adc var_Temp_Pattern
	sta var_Temp_Pattern
	lda #$00
	tay
	adc var_Temp_Pattern + 1
	sta var_Temp_Pattern + 1
:
	dec var_Temp2						; Next row
	bne @RowLoop

@Finished:
	clc
	tya
	adc var_Temp_Pattern
	sta var_ch_PatternAddrLo, x
	lda #$00
	adc var_Temp_Pattern + 1
	sta var_ch_PatternAddrHi, x
	lda var_Temp3
	bmi :+
	jsr ft_load_instrument
:
@ChannelLoopEpilog:
	CH_LOOP_END @ChannelLoop

	lda #$00
	sta var_SkipTo
	rts

@Effect:
	cmp #$80
	beq @LoadInstCmd
	cmp #$84
	beq @EffectDuration
	cmp #$86
	beq @EffectNoDuration
	cmp #$F0							; See if volume
	bcs @OneByteCommand
	cmp #$E0							; See if a quick instrument command
	bcs @LoadInst
;	cmp #$8E
;	beq @OneByteCommand
	cmp #$94
	beq @OneByteCommand
	cmp #$A4
	beq @OneByteCommand
	iny									; Command takes two bytes
@OneByteCommand:						; Command takes one byte
	iny
	rts									; A new command or note is immediately following
@EffectDuration:
	iny
	lda (var_Temp_Pattern), y
	iny
	sta var_ch_DefaultDelay, x
	rts
@EffectNoDuration:
	iny
	lda #$FF
	sta var_ch_DefaultDelay, x
	rts
@LoadInstCmd:    ; mult-byte
	iny
	lda (var_Temp_Pattern), y
	iny
	sta var_Temp3
	rts
@LoadInst:       ; single byte
	iny
	and #$0F
	asl a
	sta var_Temp3
	rts									;;; ;; ; var_ch_NoteDelay remains unaltered

.else   ; ENABLE_ROW_SKIP
	rts
.endif  ; ENABLE_ROW_SKIP
