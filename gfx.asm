.cpu _45gs02

#import "mega65_std.asm"

.segment Virtual
RLE_Data: {

* = $02 "RLE decoder data" virtual
state:		.byte 0
tmp1:		.byte 0
tmp2:	  	.byte 0
x_count:	.word 0
data_ptr:	.word 0
dest_ptr:	.word 0
color_strip:	.fill 128, 0

}

* = $02 "Basepage" virtual
Q1:             .word $0000
		.word $0000
Q2:		.word $0000
		.word $0000
Q3:		.word $0000
		.word $0000


.segment Code "gfx"

InitGfx: {
                // Set color ram for first 16 rows
                StW($02, $d800)
                ldz #$00
                ldy #16
!row:  	        ldx #20
!col:		lda #$08
                sta ($02),z
                inw $02
                lda #$0f
                sta ($02),z
                inw $02
                dex
                bne !col-
                dey
                bne !row-

                // Screen memory $0000400
                lda #$00
                sta $d063
                sta $d062
                lda #$04
                sta $d061
                lda #$00
                sta $d060

                // Char data at $40000
                lda #$04
                sta $d06a
                lda #$00
                sta $d069
                sta $d068

                // Select color palette 3 for text and map it
                lda #%11111111
                sta $d070


                // Load palette data
                ldx #$00
                ldy #$00
!:              lda Colors,y
                iny
                sta $d100,x
                lda Colors,y
                iny
                sta $d200,x
                lda Colors,y
                iny
                sta $d300,x
                inx
                cpx #$10
                bne !-


                StW($02, $0400)
                StW($04, $0400)
                StW($06, $1000)

                // Fill screen ram (16bit chars: $1000, $1001, $1002, ...)
                cld
                ldx #20
!column:
                ldy #16                 // 16 rows
!:              ldz #$00
                lda $06
                sta ($02),z
                inz
                lda $07
                sta ($02),z
                inw $06
                AddByteToWord($02, 40)
                dey
                bne !-
                inw $04
                inw $04
                lda $04
                sta $02
                lda $05
                sta $03
                dex
                bne !column-

                rts
} // InitGfx


DrawImage: {
                // Init color column 
                ldx #$7f
                lda #$00
!:
                sta RLE_Data.color_strip,x
                dex
                bpl !-

                // Init state
                lda #$00
                sta RLE_Data.state		// state field (bit 0 = copy color from previous column)
                sta RLE_Data.dest_ptr
                sta RLE_Data.dest_ptr + 1
                sta RLE_Data.tmp1               // Init current color with 0
                
                StW(RLE_Data.x_count, 160)      // 320 pixels to go = 160 pixel nibbles 

                // Assume room loaded at $8000, find room image offset at $0a
                StW($02, $8000)
                ldy #$0a			// offset 0a = image data offset
                lda ($02),y
                clc
                adc $02
                sta RLE_Data.data_ptr
                iny
                lda ($02),y
                adc $03
                sta RLE_Data.data_ptr + 1

                ldz #$01			// init run length counter
} // DrawImage


ColorStripLoop:
                lda RLE_Data.tmp1

RleOddCol: {
                nop
                nop
                
                // Odd column
                ldy #$00			// y coord
col_loop:
                dez
                bne handle_pixel
                // run length counter finished
                ldx #$00
                lda (RLE_Data.data_ptr,x)	// next image byte
                inw RLE_Data.data_ptr
                cmp #$80
                bmi !+                          // < $80?
                // bit 7 set -> leave color as is
                smb0 RLE_Data.state             // set bit0 -> keep color as before
                and #$7f
                taz							
                bne handle_pixel
                ldx #$00
                lda (RLE_Data.data_ptr,x)       // zero run length counter? load from next byte
                inw RLE_Data.data_ptr
                taz
                bra handle_pixel
!:
                // bit 7 clear -> color in bits 0-3
                rmb0 RLE_Data.state     // clear bit0 -> write color from accu
                tax
                and #$0f
                sta RLE_Data.tmp1
                txa
                lsr                     // shift RLE to lower bits
                lsr
                lsr
                lsr
                bne !+                  // rle counter zero? -> take next byte
                ldx #$00
                lda (RLE_Data.data_ptr,x)
                inw RLE_Data.data_ptr
!:
                taz
                lda RLE_Data.tmp1
handle_pixel:
                bbs0 RLE_Data.state, copy_previous
                sta RLE_Data.color_strip,y      // store color in strip
                bra nexty
copy_previous:
                lda RLE_Data.color_strip,y
                lsr
                lsr
                lsr
                lsr
                sta RLE_Data.color_strip,y
nexty:
                iny
                cpy #$80
                bne col_loop		
}

RleEvenCol: {
                // Even column
                ldy #$00							// y coord
col_loop:
                dez
                bne handle_pixel
                // run length counter finished
                ldx #$00
                lda (RLE_Data.data_ptr,x)				// next image byte
                inw RLE_Data.data_ptr
                cmp #$80
                bmi !+					// < $80?
                // bit 7 set -> leave color as is
                smb0 RLE_Data.state	// set bit0 -> keep color as before
                and #$7f
                taz							
                bne handle_pixel
                ldx #$00
                lda (RLE_Data.data_ptr,x)	// zero run length counter? load from next byte
                inw RLE_Data.data_ptr
                taz
                bra handle_pixel
!:
                // bit 7 clear -> color in bits 0-3
                rmb0 RLE_Data.state             // clear bit0 -> write color from accu
                ldx #$00
                stx RLE_Data.tmp2
                sta RLE_Data.tmp1
                asw RLE_Data.tmp1               // shift so tmp1 contains color (hi)
                asw RLE_Data.tmp1               // and tmp2 contains RLE (lo)
                asw RLE_Data.tmp1
                asw RLE_Data.tmp1
                lda RLE_Data.tmp2
                bne !+
                ldx #$00
                lda (RLE_Data.data_ptr,x)
                inw RLE_Data.data_ptr
!:
                taz
                lda RLE_Data.tmp1               // keep color in A hi
handle_pixel:
                bbs0 RLE_Data.state, copy_previous
                ora RLE_Data.color_strip,y
                sta RLE_Data.color_strip,y      // store color in strip
                bra nexty
copy_previous:
                lda RLE_Data.color_strip,y
                asl
                asl
                asl 
                asl
                ora RLE_Data.color_strip,y
                sta RLE_Data.color_strip,y
nexty:
                iny
                cpy #$80
                bne col_loop
}

                // Use DMA to copy char pixel data to $a0000
                lda RLE_Data.dest_ptr
                sta DmaCopyRlePixels + $0a
                lda RLE_Data.dest_ptr + 1
                sta DmaCopyRlePixels + $0b
                DmaJobEnhanced(DmaCopyRlePixels)

                // Move to next char column after 8 double-pixel strips (16 pixels)
                inw RLE_Data.dest_ptr
                lda RLE_Data.dest_ptr
                and #%00000111
                bne !+
                AddWordToWord(RLE_Data.dest_ptr, $3f8)
!:
                dew RLE_Data.x_count
                beq !+
                jmp ColorStripLoop
!:
                rts



.segment DmaJobs "RLE DMA"
DmaCopyRlePixels:
                .byte $0a // F018A format
                .byte $85, 8  // Destination skip rate
                .byte $00 // end of job option list

                .byte $00 // copy command (no chain)
                .word 128
                .word RLE_Data.color_strip
                .byte 0
                .word 0
                .byte $04
                .word $0000 // will be ignored for simple copy job, but put here for completeness

.segment Tables
Colors:
                .byte $0, $0, $0, 	$0, $0, $A, 	$0, $A, $0, 	$0, $A, $A
    		.byte $A, $0, $0, 	$A, $0, $A, 	$A, $5, $0, 	$A, $A, $A
                .byte $5, $5, $5, 	$5, $5, $F, 	$5, $F, $5, 	$5, $F, $F
                .byte $F, $5, $5, 	$F, $5, $F, 	$F, $F, $5, 	$F, $F, $F
