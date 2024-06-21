; decompilation of NSF_CALLER_BIN and NSF_CALLER_BIN_VRC6 in Driver.h
; decompilation by Persune 2024

.ifdef ::USE_VRC6
INIT = $8000
PLAY = $8003
.else
INIT = $8008
PLAY = $800B
.endif

.org $FF80
reset:	sei
		cld
:		lda $2002
		bpl :-
:		lda $2002
		bpl :-
		ldx #0
		txa
:		sta $0200,x
		inx
		bne :-
.ifdef ::USE_VRC6
		; init banks
		lda #$00
		sta $8000
		lda #$02
		sta $C000
.endif
		lda #$0F
		sta $4015
		lda #$0A
		sta $4010
		lda #$00
		ldx #$00
		ldy #$00
		jsr INIT
		lda #$80
		sta $2000
		lda #$00
		sta $2001

:		jmp :-

nmi:	jsr PLAY
irq:	rti


.org $FFFA
vec:	.addr nmi, reset, irq
