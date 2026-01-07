;;; Memory layout breakdown:
;;;            /                                               +---------------------------+
;;;           |                                                | main_private code         |
;;;           |                                                | (0xd000)                  |
;;;           |                                         +------+---------------------------+
;;;           |                                         | screen ram  | gfx2 code          |
;;; mapped    |                                         | (0x10000)   | (0x11800)          |
;;;           |                                         +-------------+-------+------------+
;;;           |                                         | diskio code         | diskio bss |
;;;           |                                         | (0x12000)           | (0x13a00)  |
;;;           |                                         +---------------------+------------+
;;;           |                                         | gfx code        | gfx bss        |
;;;            \                                        | (0x14000)       | (0x15800)      |
;;;           |                                         +-----------------+----------------+             +---------------------------------+
;;;           |                                         | sound code                       |             | resource (room, script, ...)    |
;;;            \                                        | (0x16000)                        |             | (64 pages from 0x18000-0x27fff) |                                 
;;;            / +-----------+--------+-------+---------+----------------------------------+-------------+------------+-------------+------+------+--------------+--------------+-------+------------+--------+--------+--------+-------+-------+-------+-------+----------------+-------------------+---------------+     +-------------------+-------------------+
;;; physical  |  | registers | zzpage | CPU   | runtime | script                           | main        | heap       | backbuffer  | backbuffer  | main_private | data_sound   | soft  | screen ram | gfx2   | diskio | diskio | gfx   | gfx   | sound | sound | resource heap  | gfx/char memory   | music memory  |     | color ram         | gfx               |
;;; placement |  |           |        | stack |         | parser code                      | code        | (strings,  | screen ram  | color ram   | code         | cdata_main   | stack |            | code   | code   | bss    | code  | bss   | code  | bss   | 256 pages      | (room, objects,   |               |     |                   | helpscreen        |
;;;           |  |           |        |       |         | (M01)                            | (M02)       | inventory) |             |             | (M03)        | zdata        |       |            | (M10)  |        |        | (M12) |       |       |       | each 256 bytes |  actors)          |               |     |                   |                   |
;;;            \ +-----------+--------+-------+---------+------+------+---+---+------------+-------------+------------+-------------+------+------+--------------+--------------+-------+------------+--------+--------+--------+-------+-------+-------+-------+----------------+-------------------+---------------+ ... +-------------------+-------------------+
;;;              0x0000      0x0080   0x0100  0x0200    0x2000 0x3000 0x3800  0x3a00      0x4000         0x8000       0xa000        0xb770 0xc000 0xcee0         0xe000         0xf800  0x10000      0x11800  0x12000  0x13a80  0x14000 0x15900 0x16000 0x17800 0x18000          0x28000             0x53800   0x5ffff     0xff80800     0xff82000   0xff83fff
;;;                                                                       0x3900                        |    8 kb     |     ~6 kb   |     ~6 kb   |     ~4 kb    |     6 kb     |  2 kb |    6 kb    |  2 kb  | 6.5 kb | ~1.5 kb|  6 kb | ~2 kb |  6 kb |  2 kb |     64 kb      |       174 kb      |     50 kb     |     |        6 kb       |        8 kb       |
;;;                                                     |<----   Code Segment (CS)  ---->|              |<---    Data Segment (DS)   ---->|
;;;                                                     |        (0x2000 - 0x3fff)       |              |        (0x8000 - 0xbfff)        |
;;;              |                 8kb                  |              8 kb              |     16 kb    |              16 kb              |


(define memories
'(
        ;;;; ***********************
        ;;;; STARTUP AND INIT MEMORY
        ;;;; ***********************

        ; startup memory configuration
        ; memory for boot program (autoboot.c65)
        ; contains copy of the runtime module and will relocate it to its final memory location
        (memory autoboot (address (#x1fff . #x5fff))
                (section 
                        (autoboot_load_address #x1fff)
                        (programStart          #x2001) 
                        (startup               #x200e)
                        data_init_table
                        (runtime_copy          #x2200)
                        (section
                                code_init
                                cdata_init
                                data_init
                                               #x4000)
                )
        )

        ; temporary memory for init program (will be discarded when init is done)
        (memory bssram-init (address (#x6000 . #x7fff))
                (section
                        bss_init
                )
        )


        ;;;; ***********************
        ;;;; MEMORY LAYOUT OF BANK 0
        ;;;; ***********************

        (memory zeropage (address (#x2 . #xff))  (qualifier zpage)
	        (section (registers (#x2. #x7f)))
                (section zzpage)
        )
    
        (block stack  (size #x0100)) 
        (memory stackpage (address (#x100 . #x1ff)) 
                (section stack)
        )

        ; page aligned memory in lower 32kb space for hyppo filename transfer
        (memory bss-hyppo-xfer (address (#x200 . #x20a))
                (section
                        data_hyppo
                )
        )

        ; Memory for the runtime module
        ; The runtime code will be relocated to here during startup
        (memory m0-0 (address (#x20b . #x1fff))
                (scatter-to runtime_copy)
                (section
                        code
                        data
                        switch
                        cdata
                )
        )

        ; memory m0-1 for script module
        (memory m0-1 (address (#x2000 . #x3fff))
                (section
                        code_script
                        cdata_script
                        data_script
                )
        )

        ; memory m0-2 for main program (loaded after init is done)
        (memory m0-2 (address (#x4000 . #x7fff))
                (section
                        code_main
                        data_main
                )
        )

        ; memory for heap and back buffers
        (block heap   (size #x2000))
        (memory bss_main_lo (address (#x8000 . #xcedf))
                (section 
                        (heap              #x8000)
                        (backbuffer_screen #xa000)
                        (backbuffer_colram #xb770)
                )
        )

        ; memory for main program bss data and soft stack
        (block cstack (size #x06fa))
        (memory bss_main_hi (address (#xe380 . #xfff9))
                (section
                        zdata
                        cstack
                )
        )



        ;;;; *************************************
        ;;;; MEMORY DEFINITIONS FOR BANKED MODULES
        ;;;; *************************************

        
        ;;;; **** BANKED MEMORY code_main_private ****

        ; memory in bank 0 for mapping private vm code
        (memory main_private (address (#x2ee0 . #x3fff))
                (scatter-to bank0_cee0)
                (section
                        code_main_private
                        cdata_main_private
                        data_main_private
                )
        )
        

        ;;;; **** BANKED MEMORY gfx2 ****

        ; memory in bank 0 for mapping screenram
        (memory banked-bss-0 (address (#x2000 . #x376f)) 
                (scatter-to bank1_0000)
                (section
                        bss_screenram
                )
        )
        ; memory in bank 0 for mapping additional gfx module (gfx2)
        (memory banked-code-0 (address (#x3770 . #x3fbf)) 
                (scatter-to bank1_1770)
                (section
                        code_gfx2
                        cdata_gfx2
                        data_gfx2
                )
        )


        ;;;; **** BANKED MEMORY diskio ****

        ; memory in bank 0 for mapping diskio module
        (memory banked-code-1 (address (#x2000 . #x3fff)) 
                (scatter-to bank1_2000)
                (placement-group diskio-bits
                        (section
                                code_diskio
                                cdata_diskio
                                data_diskio
                        )
                )
                (placement-group diskio-nobits
                        (section bss_diskio)
                )
        )
 

        ;;;; **** BANKED MEMORY gfx ****

        ; memory in bank 0 for mapping gfx module
        (memory banked-code-2 (address (#x2000 . #x3fff)) 
                (scatter-to bank1_4000)
                (placement-group gfx-bits
                        (section
                                code_gfx
                                cdata_gfx
                                data_gfx
                        )
                )
                (placement-group gfx-nobits
                        (section bss_gfx)
                )
        )

        ; memory in colram for mapping gfx helpscreen module
        (memory banked-code-3 (address (#x2000 . #x3fff)) 
                (scatter-to bankc_2000)
                (placement-group gfx-helpscreen-bits
                        (section
                                code_gfx_helpscreen
                                cdata_gfx_helpscreen
                        )
                )
                (placement-group gfx-helpscreen-nobits
                        (section bss_gfx_helpscreen)
                )
        )
 

        ;;;; **** BANKED MEMORY sound ****

        ; memory in bank 0 for mapping mod sound module
        (memory banked-code-4 (address (#x2000 . #x3fff)) 
                (scatter-to bank1_6000_mod)
                (placement-group sound-mod-bits
                        (section
                                code_sound_mod
                                cdata_sound_mod
                        )
                )
                (placement-group sound-mod-nobits
                        (section bss_sound_mod)
                )
        )

        ; memory in bank 0 for mapping sid sound module
        (memory banked-code-5 (address (#x2000 . #x3fff)) 
                (scatter-to bank1_6000_sid)
                (placement-group sound-sid-bits
                        (section
                                code_sound_sid
                                data_sound_sid
                                cdata_sound_sid
                        )
                )
                (placement-group sound-sid-nobits
                        (section bss_sound_sid)
                )
        )

        ;;;; ********************************************
        ;;;; MEMORY HOLDING BANKED MODULES IN UPPER BANKS
        ;;;; ********************************************

        ; memory holding code_main_private section (will be mapped to 0x3000 during execution)
        (memory m0-3 (address (#xcee0 . #xe37f))
                (section
                        (bank0_cee0 #xcee0)
                        (data_sound_mod #xe000)
                        cdata_main
                )
        )

        (memory m1-0-bss (address (#x10000 . #x117ff))
                (section 
                        (bank1_0000 #x10000)
                )
        )

        (memory m1-0 (address (#x11770 . #x11fff))
                (section 
                        (bank1_1770 #x11770)
                        (sprites    #x11fc0)
                )
        )

        (memory m1-1 (address (#x12000 . #x13fff))
                (section bank1_2000)
        )

        (memory m1-2 (address (#x14000 . #x15fff))
                (section bank1_4000)
        )

        (memory m1-3 (address (#x16000 . #x17fff))
                (section bank1_6000_mod)
        )

        (memory m1-4 (address (#x16000 . #x17fff))
                (section bank1_6000_sid)
        )

        (memory mc-0 (address (#xff82000 . #xff83fff))
                (section bankc_2000)
        )
))
