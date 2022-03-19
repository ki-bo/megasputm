#importonce

.macro BasicUpstartMega65(addr) {
	* = $2001 "BasicUpstartMega65"

		.byte $09,$20 // End position
		.byte $72,$04 // Line number
		.byte $fe,$02,$30,$00 // BANK 0 command
		.byte <end, >end  // End of command marker (first byte after the 00 terminator)
		.byte $e2,$16 // Line number
		.byte $9e // SYS command
		.var addressAsText = toIntString(addr)
		.text addressAsText
		.byte $00
	end:
		.byte $00,$00 //End of basic terminators
}

.macro DmaJob(jobaddr) {
                lda #[jobaddr >> 16]
                sta $d702
                lda #>jobaddr
                sta $d701
                lda #<jobaddr
                sta $d700
}

.macro DmaJobEnhanced(jobaddr) {
                lda #[jobaddr >> 16]
                sta $d702
                sta $d704
                lda #>jobaddr
                sta $d701
                lda #<jobaddr
                sta $d705
}

.macro DmaCopyJobDataLoToHi(Src, Dest, Length) {
                .byte $0a // F018A format
                .byte $81, Dest >> 20
                .byte $00 // end of job option list

                .byte $00 // copy command (no chain)
                .word Length
                .word Src & $ffff
                .byte [[Src >> 16] & $0f]
                .word Dest & $ffff
                .byte [[Dest >> 16] & $0f]
                .word $0000 // will be ignored for simple copy job, but put here for completeness
}

.macro DmaFillJobDataLo(Value, Dest, Size) {
                .byte $0a // F018A format
                .byte $00 // end of job option list

                .byte $03 // Fill command
                .word Size
                .word Value
                .byte $00
                .word Dest & $ffff
                .byte [[Dest >> 16] & $0f]
                .word $0000 // ignored for non-chain job
}

.macro DmaFillJobDataHi(Value, Dest, Size) {
                .byte $0a // F018A format
                .byte $81, Dest >> 20
                .byte $00 // end of job option list

                .byte $03 // Fill command
                .word Size
                .word Value
                .byte $00
                .word Dest & $ffff
                .byte [[Dest >> 16] & $0f]
                .word $0000 // ignored for non-chain job
}

// Loads the given 32bit value into the virtual 32bit register (axyz)
.macro LoadQReg(value) {
                lda #[value]
                ldx #[value >> 8]
                ldy #[value >> 16]
                ldz #[value >> 24]
}

.macro AddByteToWord(addr, bytevalue) {
                clc
                lda addr
                adc #bytevalue
                sta addr
                bcc !+
                inc addr + 1
        !:
}

.macro StW(addr, wordvalue) {
                lda #<wordvalue
                sta addr
                lda #>wordvalue
                sta addr + 1
}

.macro AddWordToWord(addr, wordvalue) {
                clc
                lda addr
                adc #[wordvalue & $ff]
                sta addr
                lda addr + 1
                adc #[wordvalue >> 8]
                sta addr + 1
}
