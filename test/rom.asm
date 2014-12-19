*=$e000

start:
	ldx #$00

write_msg:
	jsr RCCHR
	cmp #$71		; check for 'q'
	beq end
	jsr SNDCHR
	jmp write_msg
	
SNDCHR   sta $FE                    ; Save the character to be printed
         cmp #$FF                   ; Check for a bunch of characters
         BEQ EXSC                   ; that we don't want to print to
         cmp #$00                   ; the terminal and discard them to
         beq EXSC                   ; clean up the output
         cmp #$91                   ;
         beq EXSC                   ;
         cmp #$93                   ;
         beq EXSC                   ;
         cmp #$80                   ;
         beq EXSC                   ;
         cmp #$0A                   ; Ignore line feed
         beq EXSC                   ;

GETSTS   bit $D012                  ; bit (B7) cleared yet?
         bmi GETSTS                 ; No, wait for display.
         lda $FE                    ; Restore the character
         sta $D012                  ; Output character.
EXSC     rts                        ; Return

RCCHR    lda $D011                  ; Keyboard CR
         bpl RCCHR
         lda $D010                  ; Keyboard data
         and #%01111111             ; Clear high bit to be valid ASCII
         rts

end:
	brk

msg:
	.text "HELLO, WORLD!"
	.byte $0d,$00
;	.byte $0a
;	.byte $00