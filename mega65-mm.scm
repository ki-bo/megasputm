(define memories
  '((memory mmprg
            (address (#x1fff . #xfff9)) 
            (section (loadaddress #x1fff)
                     (programStart #x2001) 
                     (startup #x200e) 
                     code 
                     data 
                     switch
                     cdata
                     (data_init_table (#x8000 . #x8fff))
                     (initcode (#x8000 . #x8fff))
                     (initcdata (#x8000 . #x8fff))
            )
    )
;    (memory upperram (address (#xe000 . #xfff9)) )
    (memory zeroPage (address (#x2 . #x7f))  (qualifier zpage)
	    (section (registers #x2)))
    (memory stackPage 
            (address (#x100 . #xfff)) 
            (section (stack #x100) 
                     cstack)
            )
    (memory bssdata
            (address (#x800 . #x1fff))
            (section zdata))
    (memory codebank1
            (address (#x10000 . #x1ffff))
    )
    (block stack (size #x100)) 
   )
)
