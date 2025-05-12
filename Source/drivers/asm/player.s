;
; ft_music_play
;
; The player routine
;
ft_music_play:
	lda var_PlayerFlags					; Skip if player is disabled
	and #%00000001						;;; ;; ; bit 0 = enable playing
	bne :+
	rts									; Not playing, return
:

.if .defined(USE_FDS)
	lda #$00
	sta var_ch_ModEffWritten
.endif

	; Run delayed channels
	ldx #$00
@ChanLoop:
	CH_LOOP_START @ChanLoopEpilog
	lda var_ch_Delay, x
	beq @SkipDelay
	dec var_ch_Delay, x
	bne @SkipDelay
	jsr ft_read_pattern					; Read the delayed note
	jmp @ChanLoopEpilog		; ;; ;;;
@SkipDelay:
	lda #$00							;;; ;; ; Clear note trigger flag
	sta var_ch_Trigger, x				; ;; ;;;
@ChanLoopEpilog:
	CH_LOOP_END @ChanLoop

	; Speed division
	lda var_Tempo_Accum + 1
	bmi ft_do_row_update				; Counter < 0
	ora var_Tempo_Accum
	beq ft_do_row_update				; Counter = 0
	jmp ft_skip_row_update
	; Read a row
ft_do_row_update:

.if .defined(USE_DPCM)
	lda #$00
	sta var_ch_DPCM_Retrig
.endif

	lda var_PlayerFlags					;;; ;; ;
	and #%00000010
	beq :+
	eor var_PlayerFlags
	and #%11111100
	sta var_PlayerFlags
:
	lda var_Load_Frame
	beq @SkipFrameLoad
	ldx #$00

	; handle delay part 1: skip over missed delay notes from the previous frame
	; this allows the driver to read all the extra commands that come after Gxx
@Delay:
	CH_LOOP_START @DelayEpilog
	lda var_ch_Delay, x
	beq @DelayEpilog
	lda #$00
	sta var_ch_Delay, x
	jsr ft_read_pattern ; skip over missed delay note
@DelayEpilog:
	CH_LOOP_END @Delay

	; Switches to new frames are delayed to next row to resolve issues with delayed notes.
	; It won't work if new pattern adresses are loaded before the delayed note is played
	lda #$00
	sta var_Load_Frame
	lda var_Current_Frame
	jsr ft_load_frame
@SkipFrameLoad:

	; handle delay part 2: skip over missed delay notes from the previous row
	; Read one row from all patterns
	ldx #$00
ft_read_channels:
	CH_LOOP_START ft_read_channels_epilog
	lda var_ch_Delay, x
	beq :+
	lda #$00
	sta var_ch_Delay, x
	jsr ft_read_pattern					; In case a delayed note has not been played, skip it to get next note
:	jsr ft_read_pattern					; Get new notes
ft_read_channels_epilog:
	CH_LOOP_END ft_read_channels

	; Should jump?
	lda var_Jump
	beq @NoJump
	; Yes, jump
	sec
	sbc #$01
	sta var_Current_Frame
;	jsr ft_load_frame
	lda #$01
	sta var_Load_Frame

	jmp @NoPatternEnd
@NoJump:
	; Should skip?
	lda var_Skip
	beq @NoSkip
	; Yes, skip
.if .defined(ENABLE_ROW_SKIP)
	; Store next row number in Temp2
	sec
	sbc #$01
	sta var_SkipTo
.endif
	jmp @NextFrame
@NoSkip:
	; Current row in all channels are processed, update info
	inc var_Pattern_Pos
	lda var_Pattern_Pos					; See if end is reached
	cmp var_Pattern_Length
	bne @NoPatternEnd
@NextFrame:								;;; ;; ; shared
	; End of current frame, load next
	lda #$01
	sta var_Load_Frame
	inc var_Current_Frame
	lda var_Current_Frame
	cmp var_Frame_Count
	bne @NoPatternEnd
	lda #$00
	sta var_Current_Frame

@NoPatternEnd:
	jsr ft_restore_speed				; Reset frame divider counter
ft_skip_row_update:
	; Speed division
	sec
	lda var_Tempo_Accum					; Decrement speed counter
	sbc var_Tempo_Count
	sta var_Tempo_Accum
	lda var_Tempo_Accum + 1
	sbc var_Tempo_Count + 1
	sta var_Tempo_Accum + 1

	ldx #$00
ft_loop_fx_state:
	CH_LOOP_START ft_loop_fx_state_epilog
	; Note cut effect (Sxx)
	lda var_ch_NoteCut, x
	beq @BeginRelease
	bpl :+					;;; ;; ;
	and #$7F
	sta var_ch_NoteCut, x
	bpl :++ ; always
:	sta var_ch_NoteCut, x
	dec var_ch_NoteCut, x
:	beq :+
	bpl @BeginRelease
:	lda #$00				; ;; ;;;
	sta var_ch_NoteCut, x
	sta var_ch_Note, x   ; todo: make a subroutine for note cut
.if .defined(USE_DPCM)
	lda ft_channel_type, x
	cmp #CHAN_DPCM
	beq @BeginRelease	; 0CC: check
	lda #$00
.endif
	sta var_ch_PortaToLo, x
	sta var_ch_PortaToHi, x
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
.if .defined(USE_VRC7)
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne @BeginRelease
	lda #$00							; Halt VRC7 channel
	sta var_ch_vrc7_Command - VRC7_OFFSET, x
.endif
@BeginRelease:
	lda var_ch_NoteRelease, x			;;; ;; ; Delayed note release
	beq @BeginTranspose
	bpl :+					;;; ;; ;
	and #$7F
	sta var_ch_NoteRelease, x
	bpl :++ ; always
:	sta var_ch_NoteRelease, x
	dec var_ch_NoteRelease, x
:	beq :+
	bpl @BeginTranspose
:	lda #$00				; ;; ;;;
	sta var_ch_NoteRelease, x
	lda var_ch_State, x
	and #STATE_RELEASE		;;; ;; ;
	bne @BeginTranspose
	ora #STATE_RELEASE
	sta var_ch_State, x
.if .defined(USE_DPCM)
	lda ft_channel_type, x
	cmp #CHAN_DPCM
	bne :+
	lda #$FF
	sta var_ch_Note, x
	bmi @BeginTranspose			; always
:
.endif
.if .defined(USE_VRC7)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_VRC7
	beq @BeginTranspose
.endif
	jsr ft_instrument_release
@BeginTranspose:
	lda var_ch_Transpose, x				;;; ;; ;
	beq @DoneTranspose
	bmi @Negative
	sec
	sbc #$10
	sta var_ch_Transpose, x
	bpl @DoneTranspose					; else var_ch_Transpose, x == #$Fx
	and #$0F
	clc
	jsr ft_clear_transpose
;	jmp @DoneTranspose
	beq @DoneTranspose					; always
@Negative:
	sec
	sbc #$10
	sta var_ch_Transpose, x
	bmi @DoneTranspose					; else var_ch_Transpose, x == #$7x
	eor #$8F
	sec
	jsr ft_clear_transpose
@DoneTranspose:
	lda var_ch_VolDelay, x
	beq :+
	cmp #$10
	bcs :+
	asl a
	asl a
	asl a
	asl a
	sta var_ch_VolDelay, x
:
ft_loop_fx_state_epilog:
	CH_LOOP_END ft_loop_fx_state		; ;; ;;;

	; Update channel instruments and effects
	ldx #$00
; Loop through wave channels
ft_loop_channels:
	CH_LOOP_START ft_loop_channels_epilog
.if .defined(USE_ALL)		;;; ;; ; Skip DPCM
	cpx #EFF_CHANS
.elseif .defined(USE_N163)
	cpx var_EffChannels
.else
	cpx #EFF_CHANS
.endif
	beq ft_update_apu

	; Do channel effects, like portamento and vibrato
	jsr ft_run_effects

	; Instrument sequences
	lda var_ch_Note, x
	beq :+
	jsr ft_run_instrument				; Update instruments
:	jsr	ft_calc_period

	;;; ;; ; Decrement volume delay counter after everything else is done
	; preferred behaviour for delayed effects?
	lda var_ch_VolDelay, x
	and #$0F
	bne :+
	lda var_ch_VolDelay, x
	and #$F0
	beq :+
	lsr a
	sta var_ch_VolColumn, x
	lda #$00
	sta var_ch_VolDelay, x
	lda var_ch_VolSlideTarget, x		;; ;; !! kill volume slide upon new volume
	bmi :+
	lda #$80
	sta var_ch_VolSlideTarget, x
	lda #$00
	sta var_ch_VolSlide, x
:
	lda var_ch_VolDelay, x
	cmp #$10
	bcc :+
	sbc #$10
	sta var_ch_VolDelay, x
:
ft_loop_channels_epilog:		; ;; ;;;
	CH_LOOP_END ft_loop_channels

ft_update_apu:
	; Finally update APU and expansion chip registers
	jsr ft_update_2a03
ft_update_ext:		;; Patch
.if .defined(USE_AUX_DATA) .and .defined(USE_ALL)
    .include "../update_ext.s"
.else
.if .defined(USE_VRC6)
	jsr	ft_update_vrc6
.endif
.if .defined(USE_VRC7)
	jsr ft_update_vrc7
.endif
.if .defined(USE_FDS)
	jsr ft_update_fds
.endif
.if .defined(USE_MMC5)
	jsr	ft_update_mmc5
.endif
.if .defined(USE_N163)
	jsr ft_update_n163
.endif
.if .defined(USE_S5B)
	jsr ft_update_s5b
.endif
.endif

END:		; End of music routine, return
	rts


; Process a pattern row in channel X
ft_read_pattern:
	ldy var_ch_NoteDelay, x				; First check if in the middle of a row delay
	beq :+
	dey
	tya
	sta var_ch_NoteDelay, x
	rts									; And skip
:	sty var_Sweep						; Y = 0
	sty var_VolumeSlideStarted			;; ;; !!
.if .defined(USE_BANKSWITCH)
	;;; ;; ; First setup the bank
	lda var_InitialBank
	beq :+
	jsr ft_bankswitch
:	; Go on
.endif
	lda var_ch_PatternAddrLo, x			; Load pattern address
	sta var_Temp_Pattern
	lda var_ch_PatternAddrHi, x
	sta var_Temp_Pattern + 1
.if .defined(USE_BANKSWITCH)
	;;; ;; ; Then etup the pattern bank
	lda var_ch_Bank, x
	beq :+
	jsr ft_bankswitch
:
.endif

ft_read_note:
	nop									;;; ;; ;
	lda (var_Temp_Pattern), y			; Read pattern command
	bpl :+
	jmp @Effect
:	bne :+								; Rest
	jmp @JumpToDone						; branch
:	cmp #$7F
;	beq @NoteOff						; Note off
	bne :+
	jsr ft_push_echo_buffer
	jmp @NoteOff
:	cmp #$7E
;	beq @NoteRelease					; Note release
	bne :+
	jmp @NoteRelease
:
	;;; ;; ; Echo buffer access
	cmp #$70
	bcc @NoEcho
	and #$0F							; sec; sbc #$70
	bne :+								; first echo
	txa									; for the tax below
	bpl :+++							; always, unless there are more than 0x80 channels
:	sta var_Temp ; = echo index
	txa
	clc
:	adc #CHANNELS
	dec var_Temp
	bne :-
:	stx var_Temp ; = channel index
	tax
	lda var_ch_EchoBuffer, x			; actually load the note
	bne :+
	ldx var_Temp
	jsr ft_push_echo_buffer				;;; ;; ;
	jmp @JumpToDone
:	ldx var_Temp
@NoEcho:
	; ;; ;;;

	; Read a note
	sta var_ch_Note, x					; Note on
	jsr ft_push_echo_buffer				;;; ;; ;
	jsr ft_translate_freq

	lda #$01							;;; ;; ; Set note trigger flag
	sta var_ch_Trigger, x				; ;; ;;;
	lda var_ch_NoteCut, x
	bmi :+
	lda #$00
	sta var_ch_NoteCut, x
:
	lda var_ch_NoteRelease, x
	bmi :+
	lda #$00
	sta var_ch_NoteRelease, x
:
.if 0
	lda var_ch_Transpose, x
	and #$F0
	beq :+
	;lda #$00
	;sta var_ch_Transpose, x
:
.endif

	lda var_ch_VolSlide, x
	bne :+
	lda var_ch_VolDefault, x
	sta var_ch_VolColumn, x
:

.if .defined(USE_DPCM)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_DPCM
	bne :+
	jsr ft_set_trigger		;;; ;; ;
	jmp @ReadIsDone
:	; DPCM skip
.endif
.if .defined(USE_VRC7)
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne :+
	jsr ft_vrc7_trigger
	jmp @RestoreDuty
:	; VRC7 skip
.endif
.if .defined(USE_S5B)
	;; ;; !!
;	if (this->m_iDefaultDuty & S5B_MODE_NOISE)
;		s_iNoiseFreq = s_iDefaultNoise;
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_S5B
	bne :+
	lda var_ch_DutyDefault + S5B_OFFSET, x
	bpl :+
	lda var_Noise_Default
	sta var_Noise_Period
:
.endif
.if 0
.if .defined(USE_N163)					;;; ;; ;
	lda ft_channel_type, x
	cmp #CHAN_N163
	bne :+
	;jsr ft_n163_load_wave2
:
.endif									; ;; ;;;
.endif
	jsr ft_reset_instrument
	jsr ft_set_trigger		;;; ;; ;
.if .defined(USE_FDS)		;;; ;; ; removed var_VolTemp
	lda ft_channel_type, x
	cmp #CHAN_FDS
	bne :+
	lda #$1F							; FDS max vol is 31
	bpl :++ ; always
:	lda #$0F							; Default max vol is 15
:
.else
	lda #$0F							; Default max vol is 15
.endif
	sta var_ch_Volume, x
;	lda #$00
;	sta var_ch_ArpeggioCycle, x
.ifdef USE_S5B		;;; ;; ;
	lda ft_channel_type, x
	cpx #CHAN_S5B
	beq @ResetSlide
.endif				; ;; ;;;
@RestoreDuty:
	lda var_ch_DutyDefault, x
	sta var_ch_DutyCurrent, x

@ResetSlide:
	; Clear the slide effect on new notes
	lda var_ch_Effect, x
	cmp #EFF_SLIDE_UP
	beq :+
	cmp #EFF_SLIDE_DOWN
	bne :++
:	lda #EFF_NONE
	sta var_ch_Effect, x
:
	cpx #APU_NOI						;;; ;; ;
	bne :+
	lda #$80 ; should be enough
	sta var_ch_TimerPeriodHi, x			; ;; ;;;
:	cpx #APU_TRI						; Skip if not square
	bcs @JumpToDone						; assume default layout
	lda #$00
	sta var_ch_Sweep, x					; Reset sweep
@JumpToDone:
	jmp @ReadIsDone
@NoteRelease:
.if .defined(USE_DPCM)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_DPCM				; ;; ;;;
	bne :+
	lda #$FF
	sta var_ch_Note, x
	jmp @ReadIsDone
:
.endif
	lda var_ch_State, x
	and #STATE_RELEASE
	bne @JumpToDone
	ora #STATE_RELEASE
	sta var_ch_State, x
.if .defined(USE_VRC7)
;	TODO: VRC7 instrument envelopes? (maybe in bhop instead)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_VRC7				; ;; ;;;
	beq @JumpToDone
.endif
	jsr ft_instrument_release
	jmp @ReadIsDone
@NoteOff:
	; VRC7 note-off handling
.if .defined(USE_VRC7)
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne :+
	lda #VRC7_HALT
	sta var_ch_vrc7_Command - VRC7_OFFSET, x
	jmp @ReadIsDone
:
.endif
	lda #$00
	sta var_ch_Note, x
.if .defined(USE_DPCM)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_DPCM				; ;; ;;;
	bne :+
	jmp @ReadIsDone
:   lda #$00
.endif
	sta var_ch_Volume, x
	sta var_ch_PortaToLo, x
	sta var_ch_PortaToHi, x
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
	cpx #$02							; Skip all but the square channels
	bcs :+
	lda #$FF
	sta var_ch_PrevFreqHigh, x
:	jmp @ReadIsDone
@VolumeCommand:							; Handle volume
	asl a
	asl a
	asl a
	;asl a
	and #$78
	sta var_ch_VolColumn, x
	sta var_ch_VolDefault, x			;;; ;; ;
	lda	var_VolumeSlideStarted			;; ;; !! kill volume slide upon new volume
	bne :+
	lda var_ch_VolSlideTarget, x
	bmi :+
	lda	#$00
	sta var_ch_VolSlide, x
	lda	#$80
	sta var_ch_VolSlideTarget, x
:	iny
	jmp ft_read_note
@InstCommand:							; Instrument change
	and #$0F
	asl a
	jsr ft_load_instrument
	iny
	jmp ft_read_note
@Effect:
	cmp #$F0							; See if volume
	bcs @VolumeCommand
	cmp #$E0							; See if a quick instrument command
	bcs @InstCommand
	asl a								; Look up the command address
	sty var_Temp						; from the command table
	tay
	lda ft_command_table, y
	sta var_Temp_Pointer
	iny
	lda ft_command_table, y
	sta var_Temp_Pointer + 1
	ldy var_Temp
	iny
	lda #>ft_read_note					;; Reloc
	pha
	lda #<ft_read_note					;; Reloc
	pha
	jmp (var_Temp_Pointer)				; And jump there
@ReadIsDone:
	lda var_ch_DefaultDelay, x			; See if there's a default delay
	cmp #$FF
	beq :+								; If so then use it
	sta var_ch_NoteDelay, x
	bne ft_read_is_done ; always
:	iny
	lda (var_Temp_Pattern), y			; A note is immediately followed by the amount of rows until next note
	sta var_ch_NoteDelay, x
ft_read_is_done:
	clc									; Store pattern address
	iny
	tya
	adc var_Temp_Pattern
	sta var_ch_PatternAddrLo, x
	lda #$00
	adc var_Temp_Pattern + 1
	sta var_ch_PatternAddrHi, x

	lda var_Sweep						; Check sweep
	beq :+
	sta var_ch_Sweep, x					; Store sweep, only used for square 1 and 2
	lda #$00
	sta var_Sweep
	sta var_ch_PrevFreqHigh, x
:	rts

; Read pattern to A and move to next byte
ft_get_pattern_byte:
	lda (var_Temp_Pattern), y			; Get the instrument number/effect bytecode
	pha
	iny
	pla
	rts

.macro pushEcho count
.if count = 0
	pla
	sta var_ch_EchoBuffer, x
.else
	lda var_ch_EchoBuffer + CHANNELS * (count - 1), x
	sta var_ch_EchoBuffer + CHANNELS * count, x
	pushEcho (count - 1)
.endif
.endmacro

ft_push_echo_buffer:		;;; ;; ; Echo buffer store
	pha
	pushEcho ECHO_BUFFER_LENGTH
	rts

;;; ;; ;
ft_set_trigger:
	lda var_ch_State, x
	and #($FF - STATE_RELEASE)
	sta var_ch_State, x
	rts

;;; ;; ; 050B
ft_set_hold:
	lda var_ch_State, x
	ora #STATE_HOLD
	sta var_ch_State, x
	rts
ft_get_hold_clear:
	lda var_ch_State, x
	and #STATE_HOLD
	pha
	eor var_ch_State, x
	sta var_ch_State, x
	pla
	rts
; ;; ;;;

;
; Command table
;
ft_command_table:
	.word ft_cmd_instrument			; 80
;	.word ft_cmd_hold
	.word ft_set_hold				; 81
	.word ft_cmd_duration			; 82
	.word ft_cmd_noduration			; 83
	.word ft_cmd_speed				; 84
	.word ft_cmd_tempo				; 85
	.word ft_cmd_jump				; 86
	.word ft_cmd_skip				; 87
	.word ft_cmd_halt				; 88
	.word ft_cmd_effvolume			; 89
	.word ft_cmd_clear				; 8A
	.word ft_cmd_porta_up			; 8B
	.word ft_cmd_porta_down			; 8C
	.word ft_cmd_portamento			; 8D
	.word ft_cmd_arpeggio			; 8E
	.word ft_cmd_vibrato			; 8F
	.word ft_cmd_tremolo			; 90
	.word ft_cmd_pitch				; 91
	.word ft_cmd_reset_pitch		; 92
	.word ft_cmd_duty				; 93
	.word ft_cmd_delay				; 94
	.word ft_cmd_sweep				; 95
	.word ft_cmd_dac				; 96
	.word ft_cmd_sample_offset		; 97
	.word ft_cmd_slide_up			; 98
	.word ft_cmd_slide_down			; 99
	.word ft_cmd_vol_slide			; 9A
	.word ft_cmd_note_cut			; 9B
	.word ft_cmd_retrigger			; 9C
	.word ft_cmd_dpcm_pitch			; 9D
	;;; ;; ;
	.word ft_cmd_note_release		; 9E
	.word ft_cmd_linear_counter		; 9F
	.word ft_cmd_groove				; A0
	.word ft_cmd_delayed_volume		; A1
	.word ft_cmd_transpose			; A2
	.word ft_cmd_phase_reset		; A3	;; ;; !!
	.word ft_cmd_DPCM_phase_reset	; A4	;; ;; !!
	.word ft_cmd_harmonic			; A5	;; ;; !!
	.word ft_cmd_target_vol_slide	; A6	;; ;; !!
.if .defined(USE_VRC7)
	.word ft_cmd_vrc7_patch_change	; A7
	.word ft_cmd_vrc7_port			; A8
	.word ft_cmd_vrc7_write			; A9
.endif
.if .defined(USE_FDS)
	.word ft_cmd_fds_mod_depth		; AA
	.word ft_cmd_fds_mod_rate_hi	; AB
	.word ft_cmd_fds_mod_rate_lo	; AC
	.word ft_cmd_fds_volume			; AD
	.word ft_cmd_fds_mod_bias		; AE
.endif
.if .defined(USE_N163)
	.word ft_cmd_n163_wave_buffer	; AF
.endif
.if .defined(USE_S5B)		;;; ;; ;
	.word ft_cmd_s5b_env_type		; B0
	.word ft_cmd_s5b_env_rate_hi	; B1
	.word ft_cmd_s5b_env_rate_lo	; B2
	.word ft_cmd_s5b_noise			; B3
.endif				; ;; ;;;
;	.word ft_cmd_expand

;
; Command functions
;

.if 0
; Loop expansion
ft_cmd_expand:
	lda var_ch_LoopCounter, x	; See if already looping
	bne :+
	; Load new loop
	jsr ft_get_pattern_byte		; number of loops
	sta var_ch_LoopCounter, x
	jsr ft_get_pattern_byte		; length in bytes
	sta var_Temp
	; Calculate pattern pointer
	sec
	lda var_Temp_Pattern
	sbc var_Temp
	sta var_Temp_Pattern
	lda var_Temp_Pattern + 1
	sbc #$00
	sta var_Temp_Pattern + 1
	ldy #$00
	jmp ft_read_note
:	; Already looping
	sec
	sbc #$01
	beq :+						; Check if done
	sta var_ch_LoopCounter, x
	iny							; number of loops, ignore
	jsr ft_get_pattern_byte		; length in bytes
	sta var_Temp
	; Calculate pattern pointer
	sec
	lda var_Temp_Pattern
	sbc var_Temp
	sta var_Temp_Pattern
	lda var_Temp_Pattern + 1
	sbc #$00
	sta var_Temp_Pattern + 1
	ldy #$00
	jmp ft_read_note
:	; Loop is done
	sta var_ch_LoopCounter, x
	iny							; number of loops, ignore
	iny							; length in bytes, ignore
	rts
.endif

; Change instrument
ft_cmd_instrument:
	jsr ft_get_pattern_byte
	jmp ft_load_instrument
;;; ;; ; 050B
;ft_cmd_hold:
;	jsr ft_set_hold
;	rts
; Set default note duration
ft_cmd_duration:
	jsr ft_get_pattern_byte
	sta var_ch_DefaultDelay, x
	rts
; No default note duration
ft_cmd_noduration:
	lda #$FF
	sta var_ch_DefaultDelay, x
	rts
; Effect: Speed (Fxx)
ft_cmd_speed:
	jsr ft_get_pattern_byte
	sta var_Speed
	lda #$00					;;; ;; ;
	sta var_GroovePointer		; ;; ;;;
	jmp ft_calculate_speed
; Effect: Tempo (Fxx)
ft_cmd_tempo:
	jsr ft_get_pattern_byte
	sta var_Tempo
	jmp ft_calculate_speed
;;; ;; ; Effect: Groove (Oxx)
ft_cmd_groove:
	jsr ft_get_pattern_byte
	sta var_GroovePointer
	lda #$00
	sta var_Speed
	jmp ft_calculate_speed
; Effect: Jump (Bxx)
ft_cmd_jump:
	jsr ft_get_pattern_byte
	sta var_Jump
	rts
; Effect: Skip (Dxx)
ft_cmd_skip:
	jsr ft_get_pattern_byte
	sta var_Skip
	rts
; Effect: Halt (Cxx)
ft_cmd_halt:
	jsr ft_get_pattern_byte
	lda var_PlayerFlags
	ora #%00000010
	sta var_PlayerFlags
	rts
;;; ;; ; Effect: Hardware envelope control (Exx)
ft_cmd_effvolume:
	jsr ft_get_pattern_byte
	bmi :+
	asl a
	asl a
	asl a
	sta var_Temp
	lda var_ch_LengthCounter, x
	and #$01
	ora #$02
	bpl :++					; always
:	and #%00000011
	sta var_Temp
	lda var_ch_LengthCounter, x
	and #%11111000
:	ora var_Temp
	sta var_ch_LengthCounter, x
	rts
; Effect: Portamento (3xx)
ft_cmd_portamento:
	jsr ft_get_pattern_byte
	sta var_ch_EffParam, x
	lda #EFF_PORTAMENTO
	sta var_ch_Effect, x
	rts
; Effect: Portamento up (1xx)
ft_cmd_porta_up:
	jsr ft_get_pattern_byte
	sta var_ch_EffParam, x
	lda #EFF_PORTA_UP
	sta var_ch_Effect, x
	rts
; Effect: Portamento down (2xx)
ft_cmd_porta_down:
	jsr ft_get_pattern_byte
	sta var_ch_EffParam, x
	lda #EFF_PORTA_DOWN
	sta var_ch_Effect, x
	rts
; Effect: Arpeggio (0xy)
ft_cmd_arpeggio:
	jsr ft_get_pattern_byte
	sta var_ch_EffParam, x
;	lda #$00
;	sta var_ch_ArpeggioCycle, x
	lda #EFF_ARPEGGIO
	sta var_ch_Effect, x
	rts
ft_cmd_clear:
	lda #$00
	sta var_ch_EffParam, x
	sta var_ch_Effect, x
	sta var_ch_PortaToLo, x
	sta var_ch_PortaToHi, x
	rts
; Effect: Hardware sweep (Hxy / Ixy)
ft_cmd_sweep:
	jsr ft_get_pattern_byte
	sta var_Sweep
	rts
; Effect: Vibrato (4xy)
ft_cmd_vibrato:
	jsr ft_get_pattern_byte
	pha
	lda var_ch_VibratoSpeed, x
	bne @FinishPos
.if .defined(USE_OLDVIBRATO)
	lda var_SongFlags
	and #FLAG_OLDVIBRATO
	beq :+
	lda #48
:
.endif
	sta var_ch_VibratoPos, x
@FinishPos:
	pla
	pha
	and #$F0
	sta var_ch_VibratoDepth, x
	pla
	and #$0F
	sta var_ch_VibratoSpeed, x
	rts
; Effect: Tremolo (7xy)
ft_cmd_tremolo:
	jsr ft_get_pattern_byte
	pha
	and #$F0
	sta var_ch_TremoloDepth, x
	pla
	and #$0F
	sta var_ch_TremoloSpeed, x
	cmp #$00
	beq @ResetTremolo
	rts
@ResetTremolo:					; Clear tremolo
	sta var_ch_TremoloPos, x
	rts
; Effect: Pitch (Pxx)
ft_cmd_pitch:
	jsr ft_get_pattern_byte
	sta var_ch_FinePitch, x
	rts
ft_cmd_reset_pitch:
	lda #$80
	sta var_ch_FinePitch, x
	rts
; Effect: Delay (Gxx)
ft_cmd_delay:
	pla							; discard return destination
	pla
	jsr ft_get_pattern_byte
	sta var_ch_Delay, x
	dey
	jmp ft_read_is_done
; Effect: delta counter setting (Zxx)
ft_cmd_dac:
.if .defined(USE_DPCM)
	jsr ft_get_pattern_byte
	sta var_ch_DPCMDAC
.endif
	rts
; Effect: Duty cycle (Vxx)
ft_cmd_duty:
	jsr ft_get_pattern_byte
	sta var_ch_DutyCurrent, x		;;; ;; ;
	sta var_ch_DutyDefault, x
.if .defined(USE_N163)
	lda ft_channel_type, x
	cmp #CHAN_N163
	bne :+
	jmp ft_n163_load_wave2
:
.endif
	rts
; Effect: Sample offset
ft_cmd_sample_offset:
.if .defined(USE_DPCM)
	jsr ft_get_pattern_byte
	sta var_ch_DPCM_Offset
.endif
	rts
; Effect: Slide pitch up
ft_cmd_slide_up:
	jsr ft_get_pattern_byte			; Fetch speed / note
	sta var_ch_EffParam, x
	lda #EFF_SLIDE_UP_LOAD
	sta var_ch_Effect, x
	rts
; Effect: Slide pitch down
ft_cmd_slide_down:
	jsr ft_get_pattern_byte			; Fetch speed / note
	sta var_ch_EffParam, x
	lda #EFF_SLIDE_DOWN_LOAD
	sta var_ch_Effect, x
	rts
; Effect: Volume slide
ft_cmd_vol_slide:
	jsr ft_get_pattern_byte			; Fetch speed / note
	sta var_ch_VolSlide, x
	bne :+							;;; ;; ;
	lda var_ch_VolColumn, x
	sta var_ch_VolDefault, x		; ;; ;;;
:	lda #$80						;; ;; !!
	sta var_ch_VolSlideTarget, x
	rts
; Effect: Note cut (Sxx)
ft_cmd_note_cut:
	jsr ft_get_pattern_byte
	ora #$80
	sta var_ch_NoteCut, x
	lda ft_channel_type, x
	cmp #CHAN_TRI							;;; ;; ;
	bne :+
	lda var_Linear_Counter
	ora #$80
	sta var_Linear_Counter
	lda var_ch_LengthCounter + APU_TRI
	and #%11111100
	sta var_ch_LengthCounter + APU_TRI		; ;; ;;;
:	rts
ft_cmd_linear_counter:				;;; ;; ;
	jsr ft_get_pattern_byte
	sta var_Linear_Counter
	lda var_ch_LengthCounter + APU_TRI
	ora #%00000001
	sta var_ch_LengthCounter + APU_TRI
	rts								; ;; ;;;
;;; ;; ; Effect: Note release (Lxx)
ft_cmd_note_release:
	jsr ft_get_pattern_byte
	ora #$80
	sta var_ch_NoteRelease, x
	rts
;;; ;; ; Delayed channel volume (Mxy)
ft_cmd_delayed_volume:
	jsr ft_get_pattern_byte
	sta var_ch_VolDelay, x
	rts
;;; ;; ; Effect: Delayed transpose (Txy)
ft_cmd_transpose:
	jsr ft_get_pattern_byte
	sta var_ch_Transpose, x
	rts
;; ;; !! Effect: Phase reset (=xx)
ft_cmd_phase_reset:
	jsr ft_get_pattern_byte
	bne :+		; skip if not zero
	inc var_ch_PhaseReset, x
:	rts
ft_cmd_DPCM_phase_reset:
	jsr ft_get_pattern_byte
	bne :+		; skip if not zero
	inc var_ch_DPCMPhaseReset
:	rts
;; ;; !! Effect: Frequency Multiplier (Kxx)
ft_cmd_harmonic:
	jsr ft_get_pattern_byte
	sta var_ch_Harmonic, x
	rts
;; ;; !! Effect: Target note slide (Nxy)
ft_cmd_target_vol_slide:
	jsr ft_get_pattern_byte			; Fetch speed / volume
	beq :+
	sta var_VolumeSlideStarted		; Flag just needs to be non-zero
	pha
	and #$F0
	lsr a
	lsr a
	lsr a
	lsr a
	sta var_ch_VolSlide, x
	pla
	and #$0F
	asl a
	asl a
	asl a
	sta var_ch_VolSlideTarget, x
	rts
:	sta var_ch_VolSlide, x
	sta var_ch_VolDefault, x
	lda #$80
	sta var_ch_VolSlideTarget, x
	rts
; Effect: Retrigger
ft_cmd_retrigger:
.if .defined(USE_DPCM)
	jsr ft_get_pattern_byte
	sta var_ch_DPCM_Retrig
	lda var_ch_DPCM_RetrigCntr
	bne :+
	lda var_ch_DPCM_Retrig
	sta var_ch_DPCM_RetrigCntr
:
.endif
	rts
; Effect: DPCM pitch setting
ft_cmd_dpcm_pitch:
.if .defined(USE_DPCM)
	jsr ft_get_pattern_byte
	sta var_ch_DPCM_EffPitch
.endif
	rts

; VRC7
.if .defined(USE_VRC7)
ft_cmd_vrc7_patch_change:
	jsr ft_get_pattern_byte
	sta var_ch_vrc7_EffPatch - VRC7_OFFSET, x
	rts
ft_cmd_vrc7_port:
	jsr ft_get_pattern_byte
	and #$07
	sta var_ch_vrc7_Port - VRC7_OFFSET, x
	rts
ft_cmd_vrc7_write:
	jsr ft_get_pattern_byte
	sty var_Temp
	ldy var_ch_vrc7_Port - VRC7_OFFSET, x
	sta var_ch_vrc7_Write, y
	lda bit_mask, y
	ora var_ch_vrc7_PatchFlag
	sta var_ch_vrc7_PatchFlag
	ldy var_Temp
	rts
.endif

; FDS
.if .defined(USE_FDS)
ft_cmd_fds_mod_depth:
	jsr ft_get_pattern_byte
	bmi @AutoFM					;;; ;; ;
	sta var_ch_ModEffDepth
	lda var_ch_ModEffWritten
	ora #ModEffWritten::Depth
	sta var_ch_ModEffWritten
	rts
@AutoFM:
	sta var_Temp
	lda var_ch_ModRate + 1
	bpl :+
	lda var_Temp ; using auto-fm
	ora #$80
	sta var_ch_ModRate + 1
:	rts							; ;; ;;;
ft_cmd_fds_mod_rate_hi:
	jsr ft_get_pattern_byte
	sta var_Temp
	and #$F0					;;; ;; ;
	bne @AutoFM
	lda var_Temp
	sta var_ch_ModEffRate + 1
	lda var_ch_ModEffWritten
	ora #ModEffWritten::RateHi
	sta var_ch_ModEffWritten
	lda var_ch_ModRate + 1
	bpl :+
	lda #$00
	sta var_ch_ModRate
:	rts
@AutoFM:
	lsr a
	lsr a
	lsr a
	lsr a
	ora #$80
	sta var_ch_ModRate + 1
	lda var_Temp
	and #$0F
	sta var_ch_ModRate
	inc var_ch_ModRate
	rts							; ;; ;;;
ft_cmd_fds_mod_rate_lo:
	jsr ft_get_pattern_byte
	sta var_ch_ModEffRate + 0
	lda var_ch_ModEffWritten
	ora #ModEffWritten::RateLo
	sta var_ch_ModEffWritten
	lda var_ch_ModRate + 1
	bpl :+
	lda #$00
	sta var_ch_ModRate + 1
:	rts
ft_cmd_fds_volume:		;;; ;; ;
	jsr ft_get_pattern_byte
	sta var_ch_FDSVolume
	rts					; ;; ;;;
ft_cmd_fds_mod_bias:
	jsr ft_get_pattern_byte
	sta var_ch_ModBias
	rts					; ;; ;;;
.endif

; N163
.if .defined(USE_N163)		;;; ;; ;
ft_cmd_n163_wave_buffer:
	jsr ft_get_pattern_byte
	bmi :+
	asl a
	sta var_ch_WavePos - N163_OFFSET, x
	lda var_ch_WaveLen - N163_OFFSET, x
	ora #$80
	sta var_ch_WaveLen - N163_OFFSET, x
	bmi :++		; always
:	lda var_ch_WaveLen - N163_OFFSET, x
	and #$7F
	sta var_ch_WaveLen - N163_OFFSET, x
	lda var_ch_WavePosOld - N163_OFFSET, x
	sta var_ch_WavePos - N163_OFFSET, x
:	jmp ft_n163_load_wave2
.endif						; ;; ;;;

; S5B
.if .defined(USE_S5B)		;;; ;; ;
ft_cmd_s5b_env_type:
	lda #$01
	sta var_EnvelopeTrigger
	sta var_EnvelopeEnabled
	jsr ft_get_pattern_byte
	sta var_EnvelopeType
	bne :+
	lda #$00
	sta var_EnvelopeEnabled
	lda var_EnvelopeType
:	and #$F0
	lsr a
	lsr a
	lsr a
	lsr a
	sta var_EnvelopeAutoShift - S5B_OFFSET, x
	beq :+
	lda var_EnvelopeType
	and #$0F
	sta var_EnvelopeType
:	rts
ft_cmd_s5b_env_rate_hi:
	jsr ft_get_pattern_byte
	sta var_EnvelopeRate + 1
	rts
ft_cmd_s5b_env_rate_lo:
	jsr ft_get_pattern_byte
	sta var_EnvelopeRate
	rts
ft_cmd_s5b_noise:
;	case EF_SUNSOFT_NOISE: // W
;		s_iDefaultNoise = s_iNoiseFreq = EffParam & 0x1F;		// // // 050B
	jsr ft_get_pattern_byte
	and #$1F
	sta var_Noise_Period
	sta var_Noise_Default
	rts
.endif						; ;; ;;;

;
; End of commands
;

ft_load_freq_table:
	lda ft_channel_type, x
.if .defined(USE_N163)
	cmp #CHAN_N163
	beq ft_load_n163_table
.endif
.if .defined(USE_FDS)
	cmp #CHAN_FDS
	beq ft_load_fds_table
.endif
.if .defined(USE_VRC6)
	cmp #CHAN_SAW
	beq ft_load_saw_table
.endif
	; fallthrough

.if .defined(PAL_PERIOD_TABLE)
	lda var_SongFlags
	and #FLAG_USEPAL
	bne ft_load_pal_table
.endif

.if .defined(NTSC_PERIOD_TABLE)
ft_load_ntsc_table:
	lda	#<ft_periods_ntsc		;; Reloc
	sta var_Note_Table
	lda #>ft_periods_ntsc		;; Reloc
	sta var_Note_Table + 1
	rts
.endif

.if .defined(PAL_PERIOD_TABLE)
ft_load_pal_table:
	lda	#<ft_periods_pal		;; Reloc
	sta var_Note_Table
	lda #>ft_periods_pal		;; Reloc
	sta var_Note_Table + 1
	rts
.endif

.if .defined(USE_N163)
ft_load_n163_table:
	lda #<ft_periods_n163		;; Reloc
	sta var_Note_Table
	lda #>ft_periods_n163		;; Reloc
	sta var_Note_Table + 1
	rts
.endif

.if .defined(USE_VRC6)
ft_load_saw_table:
	lda #<ft_periods_sawtooth	;; Reloc
	sta var_Note_Table
	lda #>ft_periods_sawtooth	;; Reloc
	sta var_Note_Table + 1
	rts
.endif

.if .defined(USE_FDS)
ft_load_fds_table:
	lda #<ft_periods_fds		;; Reloc
	sta var_Note_Table
	lda #>ft_periods_fds		;; Reloc
	sta var_Note_Table + 1
	rts
.endif									; ;; ;;;

ft_clear_transpose:						;;; ;; ;
	adc var_ch_Note, x
	sta var_ch_Note, x
	sta var_ch_EchoBuffer, x
	jsr ft_translate_freq_only
	lda #$00
	sta var_ch_Transpose, x
;	sta var_ch_Effect, x
	rts									; ;; ;;;

;
; Translate the note in A to a frequency and stores in current channel
; Don't care if portamento is enabled
;
ft_translate_freq_only:

	sec
	sbc #$01
	
	cpx #APU_NOI							;;; ;; ; Check if noise
	beq StoreNoise2

	pha						
.if .defined(USE_LINEARPITCH)				;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	bne LoadFrequencyLinear
.endif										; ;; ;;;
.if .defined(USE_VRC7)
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne :+
	pla
	sta var_ch_vrc7_ActiveNote - VRC7_OFFSET, x
	jmp ft_vrc7_get_freq_only
:
.endif

.if .defined(USE_N163) || .defined(USE_FDS) || .defined(USE_VRC6)		;;; ;; ;
	jsr	ft_load_freq_table
.endif							; ;; ;;;
	pla
	asl a
	sty var_Temp

	tay
LoadFrequency:
	lda (var_Note_Table), y
	sta var_ch_TimerPeriodLo, x
	iny
	lda (var_Note_Table), y
	sta var_ch_TimerPeriodHi, x
	ldy var_Temp
	rts

.if .defined(USE_LINEARPITCH)				;;; ;; ;
LoadFrequencyLinear:		;;; ;; ;
	lda #$00
	sta var_ch_TimerPeriodLo, x
	pla
	lsr a
	ror var_ch_TimerPeriodLo, x
	lsr a
	ror var_ch_TimerPeriodLo, x
	lsr a
	ror var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
	rts
.endif

StoreNoise2:
;    eor #$0F
.if .defined(SCALE_NOISE)
	asl a
	asl a
	asl a
	asl a
.endif
.if 0
	and #$0F
	ora #$10
	sta var_ch_TimerPeriodLo, x
	lda #$00
	sta var_ch_TimerPeriodHi, x
.endif
	sta var_ch_TimerPeriodLo, x					;;; ;; ;
	rts

;
; Translate the note in A to a frequency and stores in current channel
; If portamento is enabled, store in PortaTo
;

ft_translate_freq:

	sec
	sbc #$01

	cpx #APU_NOI				;;; ;; ; Check if noise
	beq @Noise

	pha
.if .defined(USE_DPCM)
	lda ft_channel_type, x		;;; ;; ;
	cmp #CHAN_DPCM				; ;; ;;;
	bne :+
	jmp StoreDPCM
:
.endif

.if .defined(USE_LINEARPITCH)				;;; ;; ;
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :+
	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne LoadFrequencyLinear
	lda #$00
	sta var_ch_PortaToLo, x
	pla
	lsr a
	ror var_ch_PortaToLo, x
	lsr a
	ror var_ch_PortaToLo, x
	lsr a
	ror var_ch_PortaToLo, x
	sta var_ch_PortaToHi, x
	jmp @InitPorta
:
.endif										; ;; ;;;

.if .defined(USE_VRC7)
	lda ft_channel_type, x
	cmp #CHAN_VRC7
	bne :+
	pla
	sta var_ch_vrc7_ActiveNote - VRC7_OFFSET, x
	jmp ft_vrc7_get_freq
:
.endif

.if .defined(USE_N163) || .defined(USE_FDS) || .defined(USE_VRC6)		;;; ;; ;
	jsr	ft_load_freq_table
.endif							; ;; ;;;
	pla
	asl a
	sty var_Temp
	tay
	; Check portamento
	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	beq :+
	jmp LoadFrequency
	; Load portamento
:	lda (var_Note_Table), y
	sta var_ch_PortaToLo, x
	iny
	lda (var_Note_Table), y
	sta var_ch_PortaToHi, x

	ldy var_Temp
	jmp	@InitPorta
@Noise:								; Special case for noise
.if .defined(SCALE_NOISE)
	asl a
	asl a
	asl a
	asl a
.endif
;    eor #$0F
	ora #$10						;;; ;; ; 0CC: check
	pha
	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne @NoPorta
	pla
	sta var_ch_PortaToLo, x
	lda #$00
	sta var_ch_PortaToHi, x
@InitPorta:
	lda var_ch_TimerPeriodLo, x
	ora var_ch_TimerPeriodHi, x
	bne :+
	lda var_ch_PortaToLo, x
	sta var_ch_TimerPeriodLo, x
	lda var_ch_PortaToHi, x
	sta var_ch_TimerPeriodHi, x
:	rts
@NoPorta:
	pla
	sta var_ch_TimerPeriodLo, x
	lda #$00
	sta var_ch_TimerPeriodHi, x
	rts

.if .defined(USE_DPCM)
StoreDPCM:							; Special case for DPCM

	clc                             ; Multiply the DPCM instrument index by 3
	pla                             ; and store in Temp16
	pha
	asl a
	adc var_dpcm_inst_list
	sta var_Temp16
	lda #$00
	adc var_dpcm_inst_list + 1
	sta var_Temp16 + 1
	clc
	pla
	adc var_Temp16
	sta var_Temp16
	lda #$00
	adc var_Temp16 + 1
	sta var_Temp16 + 1

	sty var_Temp
	ldy #$00

	lda (var_Temp16), y				; Read pitch
	sta var_ch_SamplePitch
	iny
	lda var_ch_DPCMDAC
	bpl :+
	lda (var_Temp16), y             ; Read delta value
	bmi :+
	sta var_ch_DPCMDAC
:	iny
	lda (var_Temp16), y				; Read sample
	tay

	lda var_dpcm_pointers			; Load sample pointer list
	sta var_Temp16
	lda var_dpcm_pointers + 1
	sta var_Temp16 + 1

	lda (var_Temp16), y				; Sample address
	sta var_ch_SamplePtr
	iny
	lda (var_Temp16), y				; Sample size
	sta var_ch_SampleLen
	iny
	lda (var_Temp16), y				; Sample bank
	sta var_ch_SampleBank

	ldy var_Temp

	; Reload retrigger counter
	lda var_ch_DPCM_Retrig
	sta var_ch_DPCM_RetrigCntr

	rts
.endif

ft_limit_note:		;;; ;; ;
;	pha
;	lda ft_channel_type, x
;	cmp #CHAN_NOI
;	pla
	cpx #APU_NOI
	beq :+++
	cmp #$00	; no note
	beq :+
	bpl :++
:	lda #$01
:	cmp #$60	; note 95 (incremented by one earlier)
	bcc :+
	lda #$60
:	rts				; ;; ;;;

.if .defined(USE_LINEARPITCH)
ft_linear_prescale:
	lda var_ch_PeriodCalcLo, x
	asl a
	rol var_ch_PeriodCalcHi, x
	asl a
	rol var_ch_PeriodCalcHi, x
	asl a
	rol var_ch_PeriodCalcHi, x
	sta var_ch_PeriodCalcLo, x
	sta var_Temp
	rts

ft_linear_fetch_pitch:
	jsr ft_linear_prescale
	asl var_ch_PeriodCalcHi, x
	
	ldy var_ch_PeriodCalcHi, x
	lda (var_Note_Table), y
	sta var_ch_PeriodCalcLo, x
	iny
	lda (var_Note_Table), y
	sta var_ch_PeriodCalcHi, x

	cpy #$BF
	bcs :+	;Return
	lda var_Temp
	beq :+	;Return

	lda ft_channel_type, x
	cmp #CHAN_FDS
	beq @FrequencyReg
;	cmp #CHAN_VRC7
;	beq @FrequencyReg
	cmp #CHAN_N163
	beq @FrequencyReg

	sec
	iny
	lda var_ch_PeriodCalcLo, x
	sbc (var_Note_Table), y
	sta var_Temp16
	iny
	lda var_ch_PeriodCalcHi, x
	sbc (var_Note_Table), y
	sta var_Temp16 + 1

	jsr ft_correct_finepitch

	sec
	lda var_ch_PeriodCalcLo, x
	sbc ACC + 1
	sta var_ch_PeriodCalcLo, x
	lda var_ch_PeriodCalcHi, x
	sbc EXT
	sta var_ch_PeriodCalcHi, x
	jmp :+	;Return
@FrequencyReg:
	sec
	iny
	lda (var_Note_Table), y
	sbc var_ch_PeriodCalcLo, x
	sta var_Temp16
	iny
	lda (var_Note_Table), y
	sbc var_ch_PeriodCalcHi, x
	sta var_Temp16 + 1

ft_linear__final:
	jsr ft_correct_finepitch

	clc
	lda var_ch_PeriodCalcLo, x
	adc ACC + 1
	sta var_ch_PeriodCalcLo, x
	lda var_ch_PeriodCalcHi, x
	adc EXT
	sta var_ch_PeriodCalcHi, x
:
	rts

ft_correct_finepitch:
	jsr MUL
	lda ACC + 1
	ora EXT
	bne :+
	inc ACC + 1
:	rts
.endif

;;; ;; ; Obtain current speed value
ft_fetch_speed:
	lda var_Speed
	bne :+
	lda var_Groove_Table
	sta var_Temp_Pointer
	lda var_Groove_Table + 1
	sta var_Temp_Pointer + 1
	ldy var_GroovePointer
	lda (var_Temp_Pointer), y
:	rts
; ;; ;;;

; Reload speed division counter
ft_restore_speed:
	lda var_Tempo				;;; ;; ;
	bne :+
	sta var_Tempo_Accum + 1
	jsr ft_fetch_speed
	sta var_Tempo_Accum
	bne :++						; ;; ;;; always branches
:	clc
	lda var_Tempo_Accum
	adc var_Tempo_Dec
	sta var_Tempo_Accum
	lda var_Tempo_Accum + 1
	adc var_Tempo_Dec + 1
	sta var_Tempo_Accum + 1
	sec
	lda var_Tempo_Accum
	sbc var_Tempo_Modulus
	sta var_Tempo_Accum
	lda var_Tempo_Accum + 1
	sbc var_Tempo_Modulus + 1
	sta var_Tempo_Accum + 1
:	lda var_GroovePointer		;;; ;; ; Move groove pointer
	beq :+
	jsr ft_calculate_speed
	inc var_GroovePointer
	ldy var_GroovePointer
	lda var_Groove_Table
	sta var_Temp_Pointer
	lda var_Groove_Table + 1
	sta var_Temp_Pointer + 1
	lda (var_Temp_Pointer), y	; load entry
	bne :+						; do not branch if groove table reaches end
	iny							; load next byte
	lda (var_Temp_Pointer), y
	sta var_GroovePointer		; ;; ;;;
:	rts

; Calculate frame division from the speed and tempo settings
ft_calculate_speed:
	tya
	pha

	lda var_Tempo
	bne :+						;;; ;; ; zero tempo -> use speed as tick
	lda #$01
	sta var_Tempo_Count
	lda #$00
	sta var_Tempo_Count + 1
	sta var_Tempo_Modulus
	sta var_Tempo_Modulus + 1
	jmp @EndCalc				; ;; ;;;
:
	; Multiply by 24
.if .defined(USE_MMC5) && .defined(USE_MMC5_MULTIPLIER)
	sta $5205
	lda #$18
	sta $5206
	lda $5205
	sta ACC
	lda $5206
	sta ACC + 1
.else
	sta AUX
	lda #$00
	sta AUX + 1
	ldy #$03
:	asl AUX
	rol AUX	+ 1
	dey
	bne :-
	lda AUX
	sta ACC
	lda AUX + 1
	tay
	asl AUX
	rol AUX	+ 1
	clc
	lda ACC
	adc AUX
	sta ACC
	tya
	adc AUX + 1
	sta ACC + 1
.endif

	; divide by speed
	jsr ft_fetch_speed			;;; ;; ;
	sta AUX
	lda #$00
	sta AUX + 1
	jsr DIV		; ACC/AUX -> ACC, remainder in EXT
	lda ACC
	sta var_Tempo_Count
	lda ACC + 1
	sta var_Tempo_Count + 1
	lda EXT
	sta var_Tempo_Modulus
	lda EXT + 1
	sta var_Tempo_Modulus + 1
@EndCalc:						;;; ;; ;
	pla
	tay

	rts



; Make sure the period doesn't exceed max or min
ft_limit_freq:

.if .defined(USE_LINEARPITCH)
	; linear pitch mode has different bounds check
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :+
; std::min(std::max(Period, 0), (NOTE_COUNT - 1) << LINEAR_PITCH_AMOUNT);
; even though this is a virtual function,
; nothing seems to override this in linear pitch mode
	lda #<LIMIT_PERIOD_LINEAR
	sta var_Temp16
	lda #>LIMIT_PERIOD_LINEAR
	sta var_Temp16 + 1
	jmp ft_check_limit_freq
:
.endif
ft_limit_freq_raw:
	lda ft_channel_type, x
	cmp #CHAN_NOI
	bne :+
	rts
:
	cmp #CHAN_DPCM
	bne :+
	rts
:
.if .defined(USE_N163)
	cmp #CHAN_N163
	bne :+
	; if N163, pitch ranges from 0 to $FFFF
	; there is nothing we can do
	rts
:
.endif

	tay
	lda ft_limit_freq_lo, y
	sta var_Temp16
	lda ft_limit_freq_hi, y
	sta var_Temp16 + 1

ft_check_limit_freq:
	lda var_ch_TimerPeriodHi, x
	bmi @LimitMin
	sec
	lda var_ch_TimerPeriodLo, x
	sbc var_Temp16
	lda var_ch_TimerPeriodHi, x
	sbc var_Temp16 + 1
	bcc @LimitEnd
@LimitMax:
	lda var_Temp16 + 1
	sta var_ch_TimerPeriodHi, x
	lda var_Temp16
	sta var_ch_TimerPeriodLo, x
	jmp @LimitEnd
@LimitMin:
	lda #$00
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x
@LimitEnd:
	rts


; same as above but for var_ch_PeriodCalcHi/var_ch_PeriodCalcLo
; roughly correlates to CChannelHandler::LimitRawPeriod()
ft_limit_final_freq:

.if .defined(USE_LINEARPITCH)
	; linear pitch mode has different bounds check
	lda var_SongFlags
	and #FLAG_LINEARPITCH
	beq :+
; std::min(std::max(Period, 0), (NOTE_COUNT - 1) << LINEAR_PITCH_AMOUNT);
; even though this is a virtual function,
; nothing seems to override this in linear pitch mode
	lda #<LIMIT_PERIOD_LINEAR
	sta var_Temp16
	lda #>LIMIT_PERIOD_LINEAR
	sta var_Temp16 + 1
	jmp ft_check_limit_final_freq
:
.endif

ft_limit_final_freq_raw:
	lda ft_channel_type, x
	cmp #CHAN_NOI
	bne :+
	rts
:
	cmp #CHAN_DPCM
	bne :+
	rts
:
.if .defined(USE_N163)
	cmp #CHAN_N163
	bne :+
	; if N163, pitch ranges from 0 to $FFFF
	; there is nothing we can do
	rts
:
.endif

	tay
	lda ft_limit_freq_lo, y
	sta var_Temp16
	lda ft_limit_freq_hi, y
	sta var_Temp16 + 1

ft_check_limit_final_freq:
	lda var_ch_PeriodCalcHi, x
	bmi @LimitMin
	sec
	lda var_ch_PeriodCalcLo, x
	sbc var_Temp16
	lda var_ch_PeriodCalcHi, x
	sbc var_Temp16 + 1
	bcc @LimitEnd
@LimitMax:
	lda var_Temp16 + 1
	sta var_ch_PeriodCalcHi, x
	lda var_Temp16
	sta var_ch_PeriodCalcLo, x
	jmp @LimitEnd
@LimitMin:
	lda #$00
	sta var_ch_PeriodCalcLo, x
	sta var_ch_PeriodCalcHi, x
@LimitEnd:
	rts


.if .defined(USE_MMC5) && .defined(USE_MMC5_MULTIPLIER)
MUL:
	lda var_Temp16
	sta $5205
	lda var_Temp
	sta $5206
	lda $5205
	sta ACC
	lda $5206
	sta ACC + 1

	lda var_Temp16 + 1
	sta $5205
	clc
	lda $5205
	adc ACC + 1
	sta ACC + 1
	lda $5206
	adc #$00
	sta EXT
	rts
.else
MUL:		;;; ;; ; var_Temp16 * var_Temp -> ACC (highest byte in EXT)
	lda #$00
	sta ACC
	sta ACC + 1
	sta EXT
	sta EXT + 1
	ldy #$08
@MultStep:
	lda var_Temp
	lsr a
	sta var_Temp
	bcc :+
	clc
	lda ACC
	adc var_Temp16
	sta ACC
	lda ACC + 1
	adc var_Temp16 + 1
	sta ACC + 1
	lda EXT
	adc EXT + 1
	sta EXT
:   asl var_Temp16
	rol var_Temp16 + 1
	rol EXT + 1
	dey
	bne @MultStep
	rts
.endif

; If anyone knows a way to calculate speed without using
; multiplication or division, please contact me

; ACC/AUX -> ACC, remainder in EXT
DIV:      LDA #0
		  STA EXT+1
		  LDY #$10
LOOP2:    ASL ACC
		  ROL ACC+1
		  ROL
		  ROL EXT+1
		  PHA
		  CMP AUX
		  LDA EXT+1
		  SBC AUX+1
		  BCC DIV2
		  STA EXT+1
		  PLA
		  SBC AUX
		  PHA
		  INC ACC
DIV2:     PLA
		  DEY
		  BNE LOOP2
		  STA EXT
		  RTS
