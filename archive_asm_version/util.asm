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

.segment DmaJobs "Global DMA jobs"
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

// Only modify Length, FillValue, DstAddr (other values are assumed unchanged)
GlobalDmaFill: {
                .byte $03 // fill command (no chain)
Length:         .word $0000 // length
                .byte $00 // ignored
FillValue:      .byte $00 // fill value
                .byte $00 // ignored
DstAddr:        .word $0000 // dst addr
                .byte $00 // dst bank
                .word $0000 // will be ignored for simple copy job, but put here for completeness
}