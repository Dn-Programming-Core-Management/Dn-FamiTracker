;
; NSF Header
;

.segment "HEADER1"
.byte 'N', 'E', 'S', 'M', $1A	; ID
.byte $01						; Version
.byte 1  						; Number of songs
.byte 1							; Start song
.word $8000						;;; ;; ; LOAD, patched in tracker
.word INIT
.word PLAY

.segment "HEADER2"
.asciiz "ft driver"				; Name, 32 bytes

.segment "HEADER3"
.asciiz ""						; Artist, 32 bytes

.segment "HEADER4"
.asciiz ""						; Copyright, 32 bytes

.segment "HEADER5"
.word $411A						; NTSC speed
.byte 0, 0, 0, 0, 0, 0, 0, 0	; Bank values
.word $4E20						; PAL speed
.byte 0   						; Flags, NTSC
.byte EXPANSION_FLAG
.byte 0, 0, 0, 0				; Reserved

;.include "driver.s"
