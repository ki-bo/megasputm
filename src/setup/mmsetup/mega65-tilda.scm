(define memories
  '(
    (block heap   (size #x1000))
  
    (memory program
            (address (#x2001 . #x9fff)) (type any)
            (section (programStart #x2001) (startup #x200e)))
    (memory zeroPage 
            (address (#x2 . #xff)) (type ram) (qualifier zpage)
	          (section (registers #x2)))
    (memory stackPage 
            (address (#x100 . #x1ff)) (type ram))
    ))
