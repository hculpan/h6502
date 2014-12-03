*=$e000

start:
	lda #$02
	ldx #$09
	
loop:
	sta $30,x
	dex
	bpl loop
	
end:
	brk