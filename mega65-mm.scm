;;; Memory layout breakdown:
;;;            /                                                      +-------------------+
;;;           |                                                       | main_private code |
;;;           |                                                       | (0xd000)          |
;;;           |                                         +-------------+-------------------+
;;;           |                                         | screen ram  | gfx2 code         |
;;; mapped    |                                         | (0x10000)   | (0x11800)         |
;;;           |                                         +-------------+------+------------+
;;;           |                                         | diskio code        | diskio bss |
;;;           |                                         | (0x12000)          | (0x13a00)  |
;;;           |                                         +-------------+------+------------+
;;;           |                                         | gfx code    | gfx bss           |
;;;            \                                        | (0x14000)   | (0x15800)         |
;;;           |                                         +-------------+------+------------+             +---------------------------------+
;;;           |                                         | sound code                      |             | resource (room, script, ...)    |
;;;            \                                        | (0x16000)                       |             | (64 pages from 0x18000-0x27fff) |                                 
;;;            / +-----------+--------+-------+---------+-------------+-------------------+-------------+------------+-------------+------+------+--------------+----------+-------+------------+--------+--------+--------+-------+-------+-------+-------+----------------+-------------------+---------------+
;;; physical  |  | registers | zzpage | CPU   | runtime | script                          | main        | heap       | backbuffer  | backbuffer  | main_private | bss      | soft  | screen ram | gfx2   | diskio | diskio | gfx   | gfx   | sound | sound | resource heap  | gfx/char memory   | sound memory  |
;;; placement |  |           |        | stack |         | parser code                     | code        | (strings,  | screen ram  | color ram   | code         | main     | stack |            | code   | code   | bss    | code  | bss   | code  | bss   | 256 pages      | (room, objects,   | (music, sfx)  |
;;;           |  |           |        |       |         | (M01)                           | (M02)       | inventory) |             |             | (M03)        |          |       |            | (M10)  |        |        | (M12) |       |       |       | each 256 bytes |  actors)          |               |
;;;            \ +-----------+--------+-------+---------+-------------+------+------------+-------------+------------+-------------+------+------+--------------+----------+-------+------------+--------+--------+--------+-------+-------+-------+-------+----------------+-------------------+---------------+
;;;              0x0000      0x0080   0x0100  0x0200    0x2000        0x3800 0x3a00      0x4000         0x8000       0xa000        0xb800 0xc000 0xd000         0xe000     0xf800  0x10000      0x11800  0x12000  0x13a00  0x14000 0x15800 0x16000 0x17800 0x18000          0x28000             0x53800   0x5ffff  
;;;                                                                                                     |    8 kb    |     6 kb    |     6 kb    |     4 kb     |   6 kb   |  2 kb |    6 kb    |  2 kb  | 6.5 kb | 1.5 kb |  6 kb | 2 kb  |  6 kb |  2 kb |     64 kb      |       174 kb      |     50 kb     |
;;;                                                     |<----   Code Segment (CS)  ---->|              |<---    Data Segment (DS)   ---->|
;;;                                                     |        (0x2000 - 0x3fff)       |              |        (0x8000 - 0xbfff)        |
;;;              |                 8kb                  |              8 kb              |     16 kb    |              16 kb              |


(define memories
'(
        ;;;; ***********************
        ;;;; MEMORY LAYOUT OF BANK 0
        ;;;; ***********************

        (block stack  (size #x0100)) 
        (block cstack (size #x07fa))
        (block heap   (size #x2000))

        (memory zeropage (address (#x2 . #xff))  (qualifier zpage)
	        (section (registers (#x2. #x7f)))
                (section zzpage)
        )
    
        (memory stackpage (address (#x100 . #x1ff)) 
                (section stack)
        )

        ; Memory m0-0 for the runtime module
        ; The runtime code will be moved to 0x200 in startup
        (memory runtime (address (#x208 . #x1fff))
                (scatter-to runtime_copy)
                (section
                        code
                        data
                        switch
                        cdata
                )        
        )

        ; memory for boot program (autoboot.c65)
        ; contains copies of m00 (runtime) and m11 (diskio) and will relocate them to their final memory locations
        (memory autoboot (address (#x1fff . #x7a7f))
                (section 
                        (autoboot_load_address #x1fff)
                        (programStart          #x2001) 
                        (startup               #x200e)
                        data_init_table
                        (runtime_copy          #x2200)
                        (init_copy             #x4000)
                        (diskio_copy           #x6000)
                )
        )

        ; memory for init program (will be discarded once executed)
        (memory init (address (#x4000 . #x5fff))
                ; we use scatter-to but not to relocate to another memory location
                ; but just to group those three sections together in one block
                (scatter-to init_copy)
                (section 
                        code_init
                        cdata_init
                        data_init
                )
        )

        ; temporary memory for init program (will be discarded when init is done)
        (memory bssram-init (address (#x6000 . #x7fff))
                (section 
                        bss_init
                )
        )

        ; memory m0-1 for script module
        (memory script (address (#x2000 . #x3fff))
                (section
                        code_script
                        cdata_script
                        data_script
                )
        )

        ; memory m0-2 for main program (loaded after init is done)
        (memory main (address (#x4000 . #x7fff))
                (section
                        code_main
                        data_main
                )
        )

        ; memory for heap, bss, and soft stack in bank 0
        (memory bssram-main (address (#x8000 . #xfff9))
                (section 
                        (heap              (#x8000 . #x9fff))
                        (backbuffer-screen (#xa000 . #xb7ff))
                        (backbuffer-color  (#xb800 . #xbfff))
                        (zdata             (#xe360 . #xf7ff))
                        (cstack            (#xf800 . #xfff9))
                )
        )



        ;;;; *************************************
        ;;;; MEMORY DEFINITIONS FOR BANKED MODULES
        ;;;; *************************************

        
        ;;;; **** BANKED MEMORY code_main_private ****

        ; memory in bank 0 for mapping private vm code
        (memory main_private (address (#x3000 . #x3fff))
                (scatter-to bank0_d000)
                (section
                        code_main_private
                        cdata_main_private
                        data_main_private
                )
        )
        

        ;;;; **** BANKED MEMORY gfx2 ****

        ; memory in bank 0 for mapping screenram
        (memory banked-bss-0 (address (#x2000 . #x37ff)) 
                (scatter-to bank1_0000)
                (section
                        bss_screenram
                )
        )
        ; memory in bank 0 for mapping additional gfx module (gfx2)
        (memory banked-code-0 (address (#x3800 . #x3fff)) 
                (scatter-to bank1_1800)
                (section
                        code_gfx2
                        cdata_gfx2
                        data_gfx2
                )
        )


        ;;;; **** BANKED MEMORY diskio ****

        ; memory in bank 0 for mapping diskio module
        (memory banked-code-1 (address (#x2000 . #x3a7f)) 
                (scatter-to diskio_copy)
                (section
                        code_diskio
                        cdata_diskio
                        data_diskio
                )
        )
        ; memory in bank 0 for mapping diskio bss section
        (memory banked-bss-1 (address (#x3a80 . #x3fff)) 
                (scatter-to bank1_3a80)
                (section
                        bss_diskio
                )
        )
 

        ;;;; **** BANKED MEMORY gfx ****

        ; memory in bank 0 for mapping gfx module
        (memory banked-code-2 (address (#x2000 . #x38ff)) 
                (scatter-to bank1_4000)
                (section
                        code_gfx
                        cdata_gfx
                        data_gfx
                )
        )
        ; memory in bank 0 for mapping gfx bss section
        (memory banked-bss-2 (address (#x3900 . #x3fff)) 
                (scatter-to bank1_5900)
                (section
                        bss_gfx
                )
        )

        ; memory in colram for mapping gfx helpscreen module
        (memory banked-code-3 (address (#x2000 . #x3fff)) 
                (scatter-to bankc_2000)
                (section
                        code_gfx_helpscreen
                        cdata_gfx_helpscreen
                )
        )
        ; memory in colram for mapping gfx helpscreen bss section
        (memory banked-bss-3 (address (#x3f00 . #x3fff)) 
                (scatter-to bankc_3f00)
                (section
                        bss_gfx_helpscreen
                )
        )
 

        ;;;; **** BANKED MEMORY sound ****

        ; memory in bank 0 for mapping sound module
        (memory banked-code-4 (address (#x2000 . #x3fff)) 
                (scatter-to bank1_6000)
                (section
                        code_sound
                        cdata_sound
                )
        )
 

        ;;;; ********************************************
        ;;;; MEMORY HOLDING BANKED MODULES IN UPPER BANKS
        ;;;; ********************************************

        ; memory holding code_main_private section (will be mapped to 0x3000 during execution)
        (memory m0-3 (address (#xd000 . #xe35f))
                (section
                        (bank0_d000 #xd000)
                        (data_sound #xe000)
                        cdata_main
                )
        )

        (memory m1-0-bss (address (#x10000 . #x117ff))
                (section 
                        (bank1_0000 #x10000)
                )
        )

        (memory m1-0 (address (#x11800 . #x11fff))
                (section 
                        (bank1_1800 #x11800)
                )
        )

        (memory m1-1 (address (#x12000 . #x13fff))
                (section 
                        (bank1_3a80 #x13a80)
                )
        )

        (memory m1-2 (address (#x14000 . #x15fff))
                (section 
                        (bank1_4000 #x14000)
                        (bank1_5900 #x15900)
                )
        )

        (memory m1-3 (address (#x16000 . #x17fff))
                (section 
                        (bank1_6000 #x16000)
                )
        )

        (memory mc-0 (address (#xff82000 . #xff83fff))
                (section
                        (bankc_2000 #xff82000)
                        (bankc_3f00 #xff83f00)
                )
        )
))
