.cpu _45gs02

.segment Virtual
* = $02 "Floppy Vars" virtual
FilenamePtr:    .word $0000
ReadSecondHalf: .byte $00
ReadPtr:        .word $0000
NextTrack:      .byte $00
NextSector:     .byte $00
QTrkSecList:    .fill 4,0
TmpByte:        .byte $00

* = $200 "Floppy buffer" virtual
SectorBuffer:   .fill 512, 0

.segment Code "Floppy Loader"
Floppy: {

ErrorJmp:
                jmp Error

InitFileTracksAndSectors: {
                // map sector data to $de00
                lda #$81
                sta $d680

                // See floppy buffer, not SD buffer
                lda #$80
	        trb $d689

                // Init track/sector list with 0s
                LoadQReg($4ff00)
                stq QTrkSecList
                ldz #54*2 - 1
!:
                lda #$00
                sta ((QTrkSecList)),z
                dez
                bpl !-

                // Load directory
                ldx #40
                ldy #00
                jsr ReadSector
                bcs ErrorJmp

                ldx $de00
                ldy $de01
                jsr ReadSector
                bcs ErrorJmp

ReadFilenames:
                ldy #$00
                StW(ReadPtr, $de00)
                bbr0 ReadSecondHalf,!+
                inc ReadPtr + 1 // odd sector number, data at $df00
!:
                iny
                lda (ReadPtr),y
                sta NextSector  // remember next directory sector
                bra !+
FilenameLoop:
                // skip first two bytes
                iny
!:
                iny
                lda (ReadPtr),y
                iny
                cmp #$82 // PRG file type
                bne NextFilename
                lda (ReadPtr),y
                beq NextFilename // Skip if file's track is 0
                iny
                sta FileTrack
                lda (ReadPtr),y
                cmp #40 // invalid sector number
                bpl NextFilename
                iny
                sta FileSector

FilenameCmp:
                lda (ReadPtr),y
                iny
                // check for number
                cmp #$30 // petscii '0'
                bmi NextFilename
                cmp #$40 // petscii '9' + 1
                bpl NextFilename
                and #$0f // convert ascii to number
                sta LflFilename

                lda (ReadPtr),y
                iny
                // check for number
                cmp #$30 // petscii '0'
                bmi NextFilename
                cmp #$40 // petscii '9' + 1
                bpl NextFilename
                and #$0f
                sta LflFilename + 1

                // compare remaining filename bytes 'xx.LFL'
                ldx #$02
!:
                lda (ReadPtr),y
                cmp #$a0
                beq FilenameMatch
                cmp LflFilename,x
                bne NextFilename
                iny
                inx
                cpx #$07
                bne !-
                bra FilenameMatch

NextFilename:
                // Jump to next multiple of 32 bytes
                tya
                and #%11100000
                clc
                adc #$20
                tay
                bne FilenameLoop
                // Sector done, load next one
                ldy NextSector
                beq EndOfDirectory // no more directory sectors

                // Next directory sector
                ldx #40
                jsr ReadSector
                bcs Error
                bra ReadFilenames // Continue reading directory entries

EndOfDirectory:
                jsr Cleanup
                rts

FilenameMatch:
                // First digit * 10 (x*8+x*2)
                lda LflFilename
                asl
                sta Times2Val
                asl
                asl
                adc Times2Val:#$00
                adc LflFilename + 1
                asl // *2 to get to byte list position
                taz 
                lda FileTrack:#$00
                sta ((QTrkSecList)),z
                inz
                lda FileSector:#$00
                sta ((QTrkSecList)),z
                bra NextFilename
} // InitFileTracksAndSectors

Error: {
                jsr Cleanup
                sec
                rts
}
                
// Turn on motor and spinup
PrepareDrive: {
                // LED and motor
                lda #$60
                sta $d080

                // spinup command
                lda #$20
                sta $d081

                jsr WaitForBusyClear
}

ReadNextSector: {
                ldx NextTrack
                ldy NextSector
                // Falltrough to ReadSector
}

// read track x (1-80) sector y (0-39)
ReadSector: {
                // logical track (1-80) to physical track (0-79)
                dex
                stx $d084

                // Find physical sector: physical = (logical / 2) + 1
                // Log  0-19 -> side 0 (y) phys 1-10 (a)
                // Log 20-39 -> side 1 (y) phys 1-10 (a)
                // side 0 by default
                tya
                ldy #$00
                sty ReadSecondHalf
                lsr             // carry = use second half of sector
                bcc !+
                inc ReadSecondHalf
!:
                inc
                cmp #$0b
                bmi !+
                // sector > 10, use side 1
                iny
                sec
                sbc #$0a
!:
                // Set physical sector
                sta $d085

                // Set side
                sty $d086

                // Sector read command (buffered mode)
                lda #$40
                sta $d081

                jsr WaitForBusyClear

                lda $d082
                // RNF or CRC error flag check
                and #$18
                bne Error

                clc
                rts
}

Cleanup: {
                // LED and motor off
                lda #$00
                sta $d080
                // Unmap sector buffer
                lda #$82
                sta $d680
                rts
}

WaitForBusyClear: {
                // bit 7? still busy
                lda $d082
                bmi WaitForBusyClear
                rts
}

//----------------------------------------

CopySectorBuffer: {
                lda #$80
	        trb $d689
                DmaJob(DmaSectorData)
                rts
}

LflFilename:
                .text "00.LFL"
                .byte $a0

} // Floppy


.segment DmaJobs "FloppyDma"
DmaSectorData:
                .byte $0a // F018A format
                .byte $80, $ff // src $ffxxxxx
                .byte $81, $00 // dst $00xxxxx
                .byte $00 // end of job option list

                .byte $00 // copy command (no chain)
                .word $0200 // size
                .word $6c00 // src $ffd6c00
                .byte $0d
                .word $0200 // dst $0000200
                .byte $00
                .word $0000 // will be ignored for simple copy job, but put here for completeness
