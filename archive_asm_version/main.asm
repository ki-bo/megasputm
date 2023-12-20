.cpu _45gs02

// Some general macros
#import "mega65_std.asm"

// Segment definitions
.file [name="mm.prg", segments="BasicUpstart,Code,DmaJobs,Tables"]
.segmentdef BasicUpstart [start=$2001]
.segmentdef Code         [start=$2016]
.segmentdef BasePage     [start=$02]
.segmentdef DmaJobs      [startAfter="Code"]
.segmentdef Tables       [startAfter="DmaJobs"]
.segmentdef GameState    [startAfter="Tables", align=$1000, max=$7fff, virtual]
.segmentdef Virtual

.segment BasePage
.zp {
tmp1:           .byte $00
tmp2:           .byte $00
tmp3:           .byte $00
tmp4:           .byte $00
tmpw1:          .word $0000
tmpw2:          .word $0000
tmpw3:          .word $0000
tmpw4:          .word $0000
tmpq1:          .dword $00000000
tmpq2:          .dword $00000000
tmpq3:          .dword $00000000
tmpq4:          .dword $00000000
}


.segment GameState

NumGlobalObjs:  .word $0000

NumRooms:       .byte $00
NumCostumes:    .byte $00
NumScripts:     .byte $00
NumSounds:      .byte $00

GlobalObjsPtr:  .word $0000
RoomsPtr:       .word $0000
CostumesPtr:    .word $0000
ScriptsPtr:     .word $0000
SoundsPtr:      .word $0000

.segment Code "Main"
Entry: {
                lbsr Init
                lbsr AttachMMd81

                lbsr InitGfx
                lbsr Floppy.InitFileTracksAndSectors

                lbsr ReadIndex

                lda #45
                ldx #$00
                ldy #$80
                lbsr Floppy.ReadRoom

                lbsr DrawImage
                jmp *
!:
                lda #$01
                sta $02
                lda #$02
                sta $02
                inc $d020
                inc $d021
                jmp !-


}
		
#import "init.asm"
#import "diskio.asm"
#import "gfx.asm"
#import "util.asm"

.segment BasicUpstart
BasicUpstartMega65(Entry)
