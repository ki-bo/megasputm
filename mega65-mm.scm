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

        ; The runtime code will be loaded to 0x200
        (memory runtime (address (#x200 . #x1fff))
                (section
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

        ; memory for init program (will be discarded once executed)
        (memory init (address (#x4000 . #x47ff))
                (scatter-to init_copy)
                (section 
                        code_init
                        cdata_init
                )
        )

        ; temporary memory for init program (will be discarded when init is done)
        (memory bssram-init (address (#x4800 . #x4fff))
                (section 
                        bss_init
                )
        )

        ; memory for main program (loaded after init is done)
        (memory main (address (#x2000 . #xcfff))
                (section
                        code_main
                        cdata_main
                        data_main
                )
        )

        ; memory for main bss
        (memory bssram-main (address (#xe000 . #xfff9))
                (section 
                        (zdata (#xe000 . #xefff))
                        (cstack (#xf000 . #xfff9))
                )
        )

        (memory banked-code-0 (address (#x2000 . #x37ff)) 
                (scatter-to bank1_2000)
                (section
                        code_diskio
                        cdata_diskio
                        data_diskio
                )
        )

        (memory banked-bss-0 (address (#x3800 . #x3fff)) 
                (scatter-to bank1_3800)
                (section
                        bss_diskio
                )
        )
 
        (memory banked-code-1 (address (#x4000 . #x5fff)) 
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
                        (bank1_3800 #x13800)
                )
        )

        (memory m1-2 (address (#x14000 . #x15fff))
                (section 
                        (bank1_4000 #x14000)
                )
        )

))
