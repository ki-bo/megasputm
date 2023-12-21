.cpu _45gs02

#import "mega65_std.asm"
#import "diskio.asm"

.segment Code "Initialization"

.align $10
TmpPage_Init: {
                .fill $10, 0
}

Init: {
                // 40 MHz
                lda #65	
                sta $00			

                // Basepage = Zeropage
                lda #$00
                tab

                // Reset MAP Banking
                lda #$00
                tax
                tay
                taz
                map

                // All RAM + I/O
                lda #$35
                sta $01

                // Enable mega65 I/O personality (eg. for VIC IV registers)
                lda #$47
                sta $d02f
                lda #$53
                sta $d02f 

                // Disable CIA IRQs
                lda #$7f
                sta $dc0d
                sta $dd0d

                // Disable Raster IRQ
                lda #$70
                sta $d01a

                // End of MAP, enables IRQs again
                eom

                // Unbank C65 ROMs
                lda #%11111000
                trb $d030

                // Disable C65 ROM write protection via Hypervisor trap
                lda #$70
                sta $d640
                clv

                // Disable hot register behaviour
                lda #%10000000
                trb $d05d

                // Black screen
                lda #$00
                sta $d020
                sta $d021

                // SEAM (CHR16) on
                lda #%00000111 
                tsb $d054

                // 40 char mode (will be 20 chars in 16 col NCM mode)
                lda #%10000000
                trb $d031

                // Reset TEXTPOS to default
                lda #$50   // lo
                sta $d04c
                lda #$0f   // hi = 0
                trb $d04d

                // advance 40 bytes per char line
                lda #40        // lo
                sta $d058
                lda #0         // hi
                sta $d059

                // display 40 chrs per row
                lda #40
                sta $d05e
                lda #%00110000  // reset upper two bits of 10 bit value
                trb $d063

                DmaJobEnhanced(ClearScreenDmaJob)
                DmaJobEnhanced(ClearColorRamDmaJob)
                
                rts
}

//-------------------------------------

.segment Code "ReadIndex"
ReadIndex: {
                .const BUFFER = $8000

                lda #$00 // index room id
                ldx #<BUFFER
                ldy #>BUFFER
                lbsr Floppy.ReadRoom

                tba
                sta RestoreBP
                lda #>TmpPage_Init
                tab

                CopyW(BUFFER + 2, NumGlobalObjs)
                StW(GlobalObjsPtr, BUFFER + 4)
                
                ldax BUFFER + 4
                adc16 NumGlobalObjs
                

                lda RestoreBP:#$00
                tab

                rts

.segment Virtual
* = <TmpPage_Init "TmpBP ReadIndex" virtual
.zp {
Counter:        .word $0000
SrcPtr:         .word $0000
}


}

//-------------------------------------

.segment Code "AttachMMd81"
AttachMMd81: {
                tay
                lda #$12
                sta $02
                lda #$34
                sta $02
                ldy #>(Hyppo_Filename)
                // hyppo_setname trap
                lda #$2e
                sta $d640
                nop
                bcc !+

                // hyppo_d81attach0 trap
                lda #$40
                sta $d640
                clv
                bcc !+

                rts

!:
                lda #$02
                sta $d020
                jmp *

}

.segment DmaJobs "DefaultDMA"
ClearScreenDmaJob:
                DmaFillJobDataLo($00, $0400, 2000)
ClearColorRamDmaJob:
                DmaFillJobDataHi($00, $ff80000, 2000)
ClearCharDmaJob:
                DmaFillJobDataLo($00, $40000, 1280)

.segment Tables "HyppoStrings"
.align $100
Hyppo_Filename:  
                .encoding "ascii"
                .text "MM.D81"
                .byte 0
