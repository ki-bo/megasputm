#importonce

.cpu _45gs02

.segment Virtual
* = $02 "Floppy Vars" virtual
ReadSecondHalf: .byte $00
ReadPtr:        .word $0000
NextTrack:      .byte $00
NextSector:     .byte $00
FileSize:       .word $0000
QTrkSecList:    .fill 4,0
QSectorData:    .fill 4,0

.segment Code "Floppy Loader"
Floppy: {

InitFileTracksAndSectors: {
                // map sector data to $de00
                lda #$81
                sta $d680

                // See floppy buffer, not SD buffer
                lda #$80
	            trb $d689

                lbsr PrepareDrive

                // Floppy controller sector buffer address
                LoadQReg($ffd6c00)
                stq QSectorData

                // Init track/sector list with 0s
                LoadQReg($4ff00)
                stq QTrkSecList
                ldz #54*2 - 1
                lda #$00
!:
                sta ((QTrkSecList)),z
                dez
                bpl !-

                // Load directory
                ldx #40 // Track 40
                ldy #00 // Sector 00
                lbsr ReadSector
                lbcs Error

                // Jump to second sector on track 40 (start of filename entries)
                ldz #00
                lda ((QSectorData)),z
                tax
                inz
                lda ((QSectorData)),z
                tay

                lbsr ReadSector
                lbcs Error

ReadFilenames:
                lda #$6c
                bbr0 ReadSecondHalf,!+
                inc // odd sector number, data at $ffd6d00
!:
                sta QSectorData+1
                ldz #$01
                lda ((QSectorData)),z
                sta NextSector  // remember next directory sector
                bra !+
FilenameLoop:
                // skip first two bytes
                inz
!:
                inz
                lda ((QSectorData)),z
                inz
                cmp #$82 // PRG file type
                bne NextFilename
                lda ((QSectorData)),z
                beq NextFilename // Skip if file's track is 0
                inz
                sta FileTrack
                lda ((QSectorData)),z
                cmp #40 // invalid sector number
                bpl NextFilename
                inz
                sta FileSector

FilenameCmp:
                lda ((QSectorData)),z
                inz
                // check for number
                cmp #$30 // petscii '0'
                bmi NextFilename
                cmp #$3a // petscii '9' + 1
                bpl NextFilename
                and #$0f // convert ascii to number
                sta LflFilename

                lda ((QSectorData)),z
                inz
                // check for number
                cmp #$30 // petscii '0'
                bmi NextFilename
                cmp #$3a // petscii '9' + 1
                bpl NextFilename
                and #$0f
                sta LflFilename + 1

                // compare remaining filename bytes 'xx.LFL'
                ldx #$02
!:
                lda ((QSectorData)),z
                cmp LflFilename,x
                bne NextFilename
                inz
                inx
                cpx #$07
                bne !-
                bra FilenameMatch

NextFilename:
                // Jump to next multiple of 32 bytes
                tza
                and #%11100000
                clc
                adc #$20
                taz
                bne FilenameLoop
                // Sector done, load next one
                ldy NextSector
                beq EndOfDirectory // no more directory sectors

                // Next directory sector
                ldx #40 // Track 40
                lbsr ReadSector
                bcs Error
                bra ReadFilenames // Continue reading directory entries

EndOfDirectory:
                lbsr Cleanup
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
                phz
                taz 
                lda FileTrack:#$00
                sta ((QTrkSecList)),z
                inz
                lda FileSector:#$00
                sta ((QTrkSecList)),z
                plz
                bra NextFilename
} // InitFileTracksAndSectors

Error: {
                lbsr Cleanup
                sec
                lda #$02
                sta $d020
                sta $d021
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

                lbsr WaitForBusyClear

                rts
}

ReadNextSector: {
                ldx NextTrack
                ldy NextSector
                // Falltrough to ReadSector
}

// read track x (1-80) sector y (0-39)
// sets ReadSecondHalf to either 
//  0 (sector data available at $ffd6c00); or
//  1 (sector data available at $ffd6d00)
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

                lbsr WaitForBusyClear

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
                rts
}

WaitForBusyClear: {
                // bit 7 set -> still busy
                lda $d082
                bmi WaitForBusyClear
                rts
}

//----------------------------------------

// Reads room (a:room no, x:<add, y:>addr)
// push room number and address to stack
ReadRoom: {
                stx DmaCopyBuffer.DstAddr
                sty DmaCopyBuffer.DstAddr + 1

                asl
                pha
                LoadQReg($4ff00)
                stq QTrkSecList
                // Floppy controller sector buffer address
                LoadQReg($ffd6c00)
                stq QSectorData
                plz // z = room number

                beq !+
                // Room size not yet read
                StW(FileSize, $0000)
                bra !++
!:
                // Room zero does not have file size, read until last sector
                StW(FileSize, $ffff)
!:
                // Init dma copy with net sector size
                lda #$fe
                sta DmaCopyBuffer.Size
                sta BlockSize

                lda ((QTrkSecList)),z
                sta NextTrack
                inz
                lda ((QTrkSecList)),z
                sta NextSector

                // See floppy buffer, not SD buffer
                lda #$80
	        trb $d689

                lbsr PrepareDrive
SectorLoop:
                lbsr ReadNextSector
                bcc !+
                rts
!:
                lda #$6c
                bbr0 ReadSecondHalf,!+
                inc
!:
                sta QSectorData + 1
                sta DmaCopyBuffer.SrcAddr + 1

                // Read next track/sector
                ldz #$00
                lda ((QSectorData)),z
                beq Finished // Next track 0 -> last sector
                sta NextTrack
                inz
                lda ((QSectorData)),z
                sta NextSector

                // Check whether file size needs to be read
                lda FileSize
                bne !+
                lda FileSize + 1
                bne !+
                // Room size still 0 -> read size from first two bytes of room file
                inz
                lda ((QSectorData)),z
                sta FileSize
                inz
                lda ((QSectorData)),z
                sta FileSize + 1
                // Two bytes already read, adjust filesize to remaining bytes
                dew FileSize
                dew FileSize
!:
                lda FileSize + 1
                bne !+
                lda FileSize
                // If FileSize == 0, reading is finished
                beq Finished
                // FileSize < 256: check whether we need to read less bytes than a complete sector
                cmp #$fe
                bpl !+
                sta DmaCopyBuffer.Size
                sta BlockSize
!:
                DmaJobEnhanced(DmaCopyBuffer)

                lda DmaCopyBuffer.DstAddr
                sta ReadPtr
                lda DmaCopyBuffer.DstAddr + 1
                sta ReadPtr + 1

                ldz #$00
!:
                lda (ReadPtr),z
                eor #$ff
                sta (ReadPtr),z
                inz
                cpz BlockSize:#$fe
                bne !-

                lda BlockSize
                cmp #$fe
                bne Finished

                AddByteToWord(DmaCopyBuffer.DstAddr, $fe)
                lbra SectorLoop

Finished:
                lbsr Cleanup
                rts
}

//----------------------------------------

LflFilename:
                .text "00.LFL"
                .byte $a0

} // Floppy


.segment DmaJobs "FloppyDma"

DmaCopyBuffer: {
                .byte $0a // F018A format
                .byte $80, $ff // src $ffxxxxx
                .byte $00 // end of job otion list

                .byte $00 // copy command (no chain)
Size:           .word $00fe // size
SrcAddr:        .word $6c02 // src $ffd6c02
                .byte $0d
DstAddr:        .word $0200 // dst $0000200
DstMB:          .byte $00
                .word $0000 // will be ignored for simple copy job, but put here for completeness
}
