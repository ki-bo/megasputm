.cpu _45gs02

// Some general macros
#import "mega65_std.asm"

// Segment definitions
.file [name="mm.prg", segments="BasicUpstart,Code,DmaJobs,Tables"]
.segmentdef BasicUpstart [start=$2001]
.segmentdef Code         [start=$2016]
.segmentdef DmaJobs      [start=$3000]
.segmentdef Tables       [start=$3a00]
.segmentdef Virtual

.segment Code "Main"
Entry: {
                lbsr Init
                lbsr AttachMMd81

                lbsr InitGfx
                lbsr Floppy.InitFileTracksAndSectors

                lda #45
                ldx #$00
                ldy #$80
                lbsr Floppy.ReadRoom

                lbsr DrawImage

                jmp *


}
		
#import "init.asm"
#import "diskio.asm"
#import "gfx.asm"
#import "util.asm"

.segment BasicUpstart
BasicUpstartMega65(Entry)
