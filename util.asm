.cpu _45gs02

#import "mega65_std.asm"

.segment Code "Util"
waitkey: {
	
!loop:          
                lda $d610  // read key
                beq !loop- // 0 = no key
                sta $d610  // write any value -> ACK key

                inc $d020

                rts
}

.segment DmaJobs "Global Lo DMA Copy"
// Only modify Length, SrcAddr, DstAddr (other values are assumed unchanged)
GlobalDmaCopyLo: {
                .byte $00 // copy command (no chain)
Length:         .word $0000 // Length
SrcAddr:        .word $0000 // Src addr
                .byte $00 // Src bank
DstAddr:        .word $0000 // Dst addr
                .byte $00 // Dst bank
                .word $0000 // will be ignored for simple copy job, but put here for completeness
}