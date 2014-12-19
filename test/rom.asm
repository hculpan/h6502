*=$e000

START:
         ldy #$01                   ; Load register Y with $01
MEM_T    lda ($22),Y                ; Load accumulator With the contents of a byte of memory
         tax                        ; Save it to X
         eor #$FF                   ; Next 4 instuctions test to see if this memory location
         sta ($22),Y                ; is ram by trying to write something new to it - new value
         cmp ($22),Y                ; gets created by XORing the old value with $FF - store the
         php                        ; result of the test on the stack to look at later
         txa                        ; Retrieve the old memory value
         sta ($22),Y                ; Put it back where it came from
         inc $22                    ; Increment $22 (for next memory location)
         bne SKP_PI                 ; Skip if we don't need to increment page
         inc $23                    ; Increment $23 (for next memory page)
SKP_PI   lda $23                    ; Get high byte of memory address
         cmp #>START                ; Did we reach start address of Tiny Basic?
         bne PULL                   ; Branch if not
         lda $22                    ; Get low byte of memory address
         cmp #<START                ; Did we reach start address of Tiny Basic?
         beq TOP                    ; If so, stop memory test so we don't overwrite ourselves
PULL  
         plp                        ; Now look at the result of the memory test
         beq MEM_T                  ; Go test the next memory location if the last one was ram
TOP
         dey                        ; If last memory location did not test as ram, decrement Y (should be $00 now)
