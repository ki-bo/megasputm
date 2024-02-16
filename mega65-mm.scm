(define memories
'(
        (block stack (size #x100)) 
        (block cstack (size #xffa))

        (memory zeropage (address (#x2 . #x7f))  (qualifier zpage)
	        (section (registers #x2))
        )
    
        (memory stackpage (address (#x100 . #x1ff)) 
                (section stack) 
        )

        ; The runtime code will be loaded to 0x200, the load address is only here to
        ; put it into the resulting raw file. These two bytes will not be loaded 
        ; into memory.
        (memory runtime (address (#x1fe . #x1fff))
                (section
                        (runtime_load_address #x1fe)
                        code
                        data
                        switch
                        cdata
                )        
        )

        ; memory for boot program
        (memory autoboot (address (#x1fff . #x2fff))
                (section 
                        (autoboot_load_address #x1fff)
                        (programStart #x2001) 
                        (startup #x200e)
                        data_init_table
                        init_copy
                )
        )

        (memory init (address (#x4000 . #x47ff))
                (scatter-to init_copy)
                (section 
                        code_init
                        cdata_init
                )
        )

        (memory bssram-init (address (#x4800 . #x4fff))
                (section 
                        bss_init
                )
        )

        ; memory for main program
        (memory main (address (#x2000 . #xcfff))
                (section
                        code_main
                )
        )

        (memory bssram-main (address (#xe000 . #xfff9))
                (section 
                        (zdata (#xe000 . #xefff))
                        (cstack (#xf000 . #xfff9))
                )
        )

        (memory banked-code-0 (address (#x2000 . #x3dff)) 
                (scatter-to bank1_2000)
                (section
                        (diskio_load_address #x2000)  ; need to skip first 2 bytes due to loading with kernal routines
                        code_diskio
                        cdata_diskio
                        data_diskio
                )
        )

        (memory banked-bss-0 (address (#x3e00 . #x3fff)) 
                (scatter-to bank1_3e00)
                (section
                        bss_diskio
                )
        )
 
        (memory banked-code-1 (address (#xa000 . #xbfff)) 
                (scatter-to bank1_4000)
                (section
                        code_gfx
                        cdata_gfx
                )
        )

        (memory m1-0 (address (#x10000 . #x11fff))
                (section 
                        (bank1_0000 #x10000)
                )
        )

        (memory m1-1 (address (#x12000 . #x13fff))
                (section 
                        (bank1_2000 #x12000)
                        (bank1_3e00 #x13e00)
                )
        )

        (memory m1-2 (address (#x14000 . #x1ffff))
                (section 
                        (bank1_4000 #x14000)
                )
        )

))
