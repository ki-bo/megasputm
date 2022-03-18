.cpu _45gs02

.segment Code "Util"
waitkey: {
	
!loop:          
                lda $d610  // read key
                beq !loop- // 0 = no key
                sta $d610  // write any value -> ACK key

                inc $d020

                rts
}
