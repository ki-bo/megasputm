;===========================================================
;Karl Jr
;===========================================================
;
; Simple object life-time management.
;
; (c) Daniel England 2022, All Rights Reserved.
;
; I intend to release this under LGPL 3?
;
;-----------------------------------------------------------
;
;
;===========================================================

  .rtmodel cpu, "*"

  .extern _Zp


  #include  "_karljr_types.inc"

  .section code

  .public    karlInit
  .public    karlASCIIToScreen
  .public    karlGetLastError
  .public    karlClearZ
  .public    karlPanic
  .public    karlIOFast
  .public    _karlModAttach
  .public    karlObjExcludeState
  .public    karlObjIncludeState
  .public    _karlObjIncStateEx
  .public    _karlObjExcStateEx

  .public    karlDefModPrepare
  .public    karlDefModInit
  .public    karlDefModChange
  .public    karlDefModRelease

  .public    _karlProcObjLst
  .public    karl_dirty
  .public    karl_changed
  .public    karl_lock
  .public    _karlCallObjLstMethod
  .public    karl_errorno


;-----------------------------------------------------------
karlInit:
;-----------------------------------------------------------
    cld

    jsr  karlIOFast

    lda  #0x04
    sta  zp:zvaltemp0
    lda  #0x00
    sta  zp:(zvaltemp0 + 1)
    sta  zp:(zvaltemp0 + 2)
    sta  zp:(zvaltemp0 + 3)

    lda  #0x00
    sta  karl_errorno

    rts


;-----------------------------------------------------------
karlASCIIToScreen:
;  .A    IN    ascii char
;  .A    OUT    screen byte
;-----------------------------------------------------------
    sta  karl_temp0
    ldy  #0x07
loop$:
    lda  karl_scrASCIIXlat, Y
    cmp  karl_temp0
    beq  subst$
    dey
    bpl  loop$

    lda  karl_temp0
    
    cmp  #0x20
    bcs  regular$

irregular$:
    lda  #0x66
    rts

regular$:
    cmp  #0x7F
    bcs  irregular$

    cmp  #0x40
    bcc  exit$
  
    cmp  #0x60
    bcc  upper$
  
    sec
    sbc  #0x60
    
    rts

upper$:
;    sec
;    sbc  #0x40
    
exit$:
    rts

subst$:
    lda  karl_scrASCIISub, Y
    rts


;-----------------------------------------------------------
karlClearZ:
;-----------------------------------------------------------
    ldz  #0x00
    rts


;-----------------------------------------------------------
karlGetLastError:
;-----------------------------------------------------------
    lda  karl_errorno

    rts


;-----------------------------------------------------------
karlPanic:
;-----------------------------------------------------------
    sei

    lda  #0x02
    sta  VIC_BRDRCLR

panic$:
    bra  panic$


;-----------------------------------------------------------
karlIOFast:
;-----------------------------------------------------------
;  Go fast
    lda  #65
    sta  0x00

;  Enable M65 enhanced registers
    lda  #0x47
    sta  0xD02F
    lda  #0x53
    sta  0xD02F

;  Switch to fast mode
;   1. C65 fast-mode enable
    lda  0xD031
    ora  #0x40
    sta  0xD031
;   2. MEGA65 40MHz enable (requires C65 or C128 fast mode 
;  to truly enable, hence the above)
    lda  #0x40
    tsb  0xD054

    rts


;-----------------------------------------------------------
_karlModAttach:
;reg0    IN    module ptr
;-----------------------------------------------------------
    phx
    phy
    phz

  #ifdef  DEBUG_RASTERTIME
    lda  #0x03
    sta  VIC_BRDRCLR
  #endif

    MvDWMem  zp:zptrself, zp:zreg0
    MvDWMem zp:zptrowner, zp:zreg0
    MvDWZ  zp:zreg0

    lda  #ERROR_MODULE
    sta  karl_errorno




    ldz #NAMEDOBJECT__name
    ldx #0x00
loopn$:
    lda [zp:zptrself], z
    jsr karlASCIIToScreen
    sta 0x0800, x
    inz
    inx
    cpx #sizeof_NAME
    bne loopn$



    ldz  #OBJECT__prepare
    ;nop
    lda  [zp:zptrself], Z
    sta  karl_proxyptr
    inz
    ;nop
    lda  [zp:zptrself], Z
    sta  karl_proxyptr + 1

    beq  exit$

    ldz  #0x00
    jsr  karlProxy

    lda  karl_errorno
    bne  exit$

  #ifdef DEBUG_RASTERTIME
    lda  #0x01
    sta  VIC_BRDRCLR
  #endif

    MvDWMem  zp:zptrself, zp:zptrowner

    lda  #ERROR_MODULE
    sta  karl_errorno

    ldz  #OBJECT__initialise
    ;nop
    lda  [zp:zptrself], Z
    sta  karl_proxyptr
    inz
    ;nop
    lda  [zp:zptrself], Z
    sta  karl_proxyptr + 1

    beq  exit$

    ldz  #0x00
    jsr  karlProxy

    lda  karl_errorno
    bne  exit$

    MvDWMem  zp:zreg0, zp:zptrowner

exit$:
    plz
    ply
    plx

    rts


;-----------------------------------------------------------
_karlObjExcStateEx:
;-----------------------------------------------------------
    lda  zp:zreg0b0
    ldx  zp:zreg0b1
    ldy  zp:zreg0b2

    phx
    phy

    jsr  karlObjExcludeState

    ply
    pla

    sei

    sta  karl_temp0
    eor  0xFF
    sta  karl_temp1

    ldz  #OBJECT__state + 1
    ;nop
    lda  [zp:zptrself], Z

    cpy  #0x00
    beq  cont0$

    and  karl_temp0
    beq  finish$

    ldy  #0x01
    sty  karl_changed

    ;nop
    lda  [zp:zptrself], Z

cont0$:
    and  karl_temp1

    ;nop
    sta  [zp:zptrself], Z

finish$:
    lda  karl_lock
    bne  exit$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

exit$:
    rts

;-----------------------------------------------------------
karlObjExcludeState:
;-----------------------------------------------------------
    cmp  #0x00
    beq  exit$

    sei

    sta  karl_temp0
    eor  #0xFF
    sta  karl_temp1

    lda  #0x00
    sta  karl_temp2

    lda  karl_temp0
    and  #(STATE_CHANGED | STATE_DIRTY | STATE_PREPARED)
    cmp  karl_temp0
    bne  begin0$

    lda  #0x01
    sta  karl_temp2

begin0$:
    ldz  #OBJECT__state
    ;nop
    lda  [zp:zptrself], Z
    and  karl_temp0
    beq  finish$

    ;nop
    lda  [zp:zptrself], Z
    sta  karl_temp0
    and  karl_temp1

    pha

    ldz  #OBJECT__oldstate
    lda  karl_temp0
    ;nop
    sta  [zp:zptrself], Z

    pla
    pha

    and  #STATE_CHANGED
    bne  cont0$

    lda  karl_temp2
    bne  cont0$

    pla
    ora  #STATE_CHANGED

    ldz  #0x01
    stz  karl_changed

    JMP  cont1$

cont0$:
    pla

cont1$:
    ldz  #OBJECT__state
    ;nop
    sta [zp:zptrself], Z

finish$:
    lda  karl_lock
    bne  exit$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

exit$:
    rts


;-----------------------------------------------------------
_karlObjIncStateEx:
;-----------------------------------------------------------
    lda  zp:zreg0b0
    ldx  zp:zreg0b1
    ldy  zp:zreg0b2

    phx
    phy

    jsr  karlObjIncludeState

    ply
    plx

    txa
    beq  exit$

    sei

    stx  karl_temp0

    ldz  #OBJECT__state + 1
    ;nop
    lda  [zp:zptrself], Z

    cpy  #0x00
    beq  cont0$

    and  karl_temp0
    bne  finish$

    ldy  #0x01
    sty  karl_changed

    ;nop
    lda  [zp:zptrself], Z

cont0$:
    ora  karl_temp0

    ;nop
    sta  [zp:zptrself], Z


finish$:
    lda  karl_lock
    bne  exit$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

exit$:
    rts


;-----------------------------------------------------------
karlObjIncludeState:
;-----------------------------------------------------------
    cmp  #0x00
    beq  exit$

    sei

    sta  karl_temp0

;    and  #STATE_DIRTY
    bit  #STATE_DIRTY
    beq  test0$

    lda  #0x01
    sta  karl_dirty

    JMP  cont0$

test0$:
;    lda  karl_temp0
;    and  #STATE_CHANGED
    bit  #STATE_CHANGED
    beq  cont0$

    lda  #0x01
    sta  karl_changed

cont0$:
    ldz  #OBJECT__state
    ;nop
    lda  [zp:zptrself], Z

    pha

    lda  karl_temp0
;    and  #(STATE_DIRTY | STATE_PREPARED)
    bit  #(STATE_DIRTY | STATE_PREPARED)
    beq  cont1$

;    lda  karl_temp0
;    and  #STATE_CHANGED
    bit  #STATE_CHANGED
    beq  update$

cont1$:
    pla
    pha

    and  #STATE_CHANGED
    bne  changed$

    ;nop
    lda  [zp:zptrself], Z
    
    ldz  #OBJECT__oldstate
    ;nop
    sta [zp:zptrself], Z

changed$:
    pla
    ora  #STATE_CHANGED
    ora  karl_temp0

    ldz  #OBJECT__state
    ;nop
    sta  [zp:zptrself], Z

    lda  #0x01
    sta  karl_changed

    JMP  finish$

update$:
    pla
    ora  karl_temp0

    ;nop
    sta  [zp:zptrself], Z

finish$:
    lda  karl_lock
    bne  exit$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

exit$:
    rts


;-----------------------------------------------------------
karlDefModPrepare:
;-----------------------------------------------------------
;halt$:
    ;inc  0xD020
    ;jmp  halt$
    
    lda  #MODULE__unitscnt
    sta  zp:zreg5b0

    lda  #MODULE__units_p
    sta  zp:zreg5b2

    lda  #OBJECT__prepare
    sta  zp:zreg5b1

    MvDWMem  zp:zreg4, zp:zptrself

    jsr  _karlCallObjLstMethod

    lda  karl_errorno
    bne  exit$

    lda #0x01
    sta 0xd020

    lda  #0x01
    sta  zp:zreg2b3
    lda  #STATE_PREPARED
    jsr  karlObjIncludeState

exit$:
    rts

;-----------------------------------------------------------
karlDefModInit:
;-----------------------------------------------------------
    lda  #MODULE__unitscnt
    sta  zp:zreg5b0

    lda  #MODULE__units_p
    sta  zp:zreg5b2

    lda  #OBJECT__initialise
    sta  zp:zreg5b1

    MvDWMem  zp:zreg4, zp:zptrself

    jsr  _karlCallObjLstMethod
    rts


;-----------------------------------------------------------
karlDefModChange:
;-----------------------------------------------------------
    rts


;-----------------------------------------------------------
karlDefModRelease:
;-----------------------------------------------------------
    rts


;-----------------------------------------------------------
karlProxy:
;-----------------------------------------------------------
    JMP  (karl_proxyptr)


;-----------------------------------------------------------
_karlCallObjLstMStore:
;-----------------------------------------------------------
    lda  karl_callcnt
    asl  a
    tax
    lda  karl_callstackidx, X
    sta  zp:zreg3wl
    lda  karl_callstackidx + 1, X
    sta  zp:(zreg3wl + 1)

    ldy  #0x00

    lda  zp:zreg2b1
    sta  (zp:zreg3wl), Y
    iny

    lda  karl_proxyptr
    sta  (zp:zreg3wl), Y
    iny

    lda  karl_proxyptr + 1
    sta  (zp:zreg3wl), Y

    rts


;-----------------------------------------------------------
_karlCallObjLstMLoad:
;-----------------------------------------------------------
    lda  karl_callcnt
    asl  a
    tax
    lda  karl_callstackidx, X
    sta  zp:zreg3wl
    lda  karl_callstackidx + 1, X
    sta  zp:(zreg3wl + 1)

    ldy  #0x00

    lda  (zp:zreg3wl), Y
    sta  zp:zreg2b1
    iny

    lda  (zp:zreg3wl), Y
    sta  karl_proxyptr
    iny

    lda  (zp:zreg3wl), Y
    sta  karl_proxyptr + 1

    rts


;-----------------------------------------------------------
_karlCallObjLstMProc:
;-----------------------------------------------------------
    lda  #ERROR_NONE
    sta  karl_errorno

    MvDWMem  zp:zptrself, zp:zreg0

    ;ldz  zp:zreg2b1
    lda zp:zreg2b1
    taz

    ;nop
    lda  [zp:zptrself], Z
    sta  karl_proxyptr
    inz
    ;nop
    lda  [zp:zptrself], Z
    sta  karl_proxyptr + 1

    beq  exit$

    jsr  karlProxy

    lda  karl_errorno
    cmp  #ERROR_ABORT
    bne  exit$

    lda  #ERROR_NONE
    sta  karl_errorno

exit$:
    rts


;-----------------------------------------------------------
_karlCallObjLstMethod:
;-----------------------------------------------------------
;  zreg4    IN    owner object
;  zreg5b1    IN    method index
;  zreg5b0    IN    list count index
;  zreg5b2    IN    owner's list index
;  zreg5b3    IN    direction
;-----------------------------------------------------------
    sei

    jsr  _karlCallObjLstMStore
    inc  karl_callcnt

  #ifdef DEBUG_RANGECHECK
    lda  karl_callcnt
    cmp  #FEATURE_CALLDEPTH
    bcc  contrc$

    jsr  karlPanic
contrc$:
  #endif

    lda  zp:zreg5b1
    sta  zp:zreg2b1

    lda  #.byte0 _karlCallObjLstMProc
    sta  zp:zreg6wl
    lda  #.byte1 _karlCallObjLstMProc
    sta  zp:(zreg6wl + 1)

    lda  karl_lock
    bne  proc$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

proc$:
    jsr  _karlProcObjLst

finish$:
    sei

    dec  karl_callcnt
    jsr  _karlCallObjLstMLoad

    lda  karl_lock
    bne  exit$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

exit$:
    rts


;-----------------------------------------------------------
_karlProcObjLstStore:
;-----------------------------------------------------------
;halt$:
;    inc  0xD020
;    JMP  halt$

    lda  karl_proccnt
    asl  a
    tax
    lda  karl_procstackidx, X
    sta  zp:zreg3wl
    lda  karl_procstackidx + 1, X
    sta  zp:(zreg3wl + 1)

    tsx
    inx
    inx
    inx
    inx
    inx
    inx
    inx

    ldy  #0x00
    lda  0x0100, X
    sta  (zp:zreg3wl), Y
    iny
    lda  0x0101, X
    sta  (zp:zreg3wl), Y

    inw  zp:zreg3wl
    inw  zp:zreg3wl

    LDQMem  zp:zptrself
    STQIndW zp:zreg3wl

    inw  zp:zreg3wl
    inw  zp:zreg3wl
    inw  zp:zreg3wl
    inw  zp:zreg3wl

    ldy  #0x00

    lda  zp:zreg2b2
    sta  (zp:zreg3wl), Y
    iny

    lda  zp:zreg5b2
    sta  (zp:zreg3wl), Y
;    iny

    inw  zp:zreg3wl
    inw  zp:zreg3wl

    LDQMem  zp:zreg7
    STQIndW zp:zreg3wl

    inw  zp:zreg3wl
    inw  zp:zreg3wl
    inw  zp:zreg3wl
    inw  zp:zreg3wl

    ldy  #0x00

    lda  zp:zreg6wh
    sta  (zp:zreg3wl), Y
    iny
    lda  zp:zreg6wh + 1
    sta  (zp:zreg3wl), Y
;    iny

    inw  zp:zreg3wl
    inw  zp:zreg3wl

    LDQMem  zp:zreg0
    STQIndW  zp:zreg3wl

    inw  zp:zreg3wl
    inw  zp:zreg3wl
    inw  zp:zreg3wl
    inw  zp:zreg3wl

    ldy  #0x00

    lda  zp:zreg2b0
    sta  (zp:zreg3wl), Y
    iny

    lda  karl_proxyptr
    sta  (zp:zreg3wl), Y
    iny

    lda  karl_proxyptr + 1
    sta  (zp:zreg3wl), Y

    rts


;-----------------------------------------------------------
_karlProcObjLstLoad:
;-----------------------------------------------------------
    lda  karl_proccnt
    asl  a
    tax
    lda  karl_procstackidx, X
    sta  zp:zreg3wl
    lda  karl_procstackidx + 1, X
    sta  zp:zreg3wl + 1

    inw  zp:zreg3wl
    inw  zp:zreg3wl

    ldz  #0x00

    LDQIndWZ  zp:zreg3wl
    STQMem  zp:zptrself

    ldz  #0x04

    lda  (zp:zreg3wl), Z
    sta  zp:zreg2b2
    inz

    lda  (zp:zreg3wl), Z
    sta  zp:zreg5b2
    inz

    LDQIndWZ  zp:zreg3wl
    STQMem  zp:zreg7

    ldz  #0x0A

    lda  (zp:zreg3wl), Z
    sta  zp:zreg6wh
    inz
    lda  (zp:zreg3wl), Z
    sta  zp:zreg6wh + 1
    inz

    LDQIndWZ  zp:zreg3wl
    STQMem  zp:zreg0

    ldz  #0x10

    lda  (zp:zreg3wl), Z
    sta  zp:zreg2b0
    inz

    lda  (zp:zreg3wl), Z
    sta  karl_proxyptr
    inz

    lda  (zp:zreg3wl), Z
    sta  karl_proxyptr + 1

    rts


;-----------------------------------------------------------
_karlProcObjLst:
;  zreg4    IN    owner object
;  zreg5b0    IN    list count
;  zreg5b2    IN    owner's list index
;  zreg6wl    IN    callback routine
;  zreg5b3    IN    direction
;-----------------------------------------------------------
    sei

    lda  #0xFF
    pha
    pha
    pha
    pha

    jsr  _karlProcObjLstStore
    inc  karl_proccnt

  #ifdef  DEBUG_RANGECHECK
    lda  karl_proccnt
    cmp  #FEATURE_CALLDEPTH
    bcc contrc$

    jsr  karlPanic
contrc$:
  #endif

    lda  zp:zreg5b3
    sta  zp:zreg2b0

    ;ldz  zp:zreg5b0
    lda  zp:zreg5b0
    taz

    ;nop
    lda  [zp:zreg4], Z

    lbeq  finish$

    sta  zp:zreg2b2

    MvDWObjMem  zp:zreg7, zp:zreg4, zp:zreg5b2
    lda  zp:zreg6wl
    sta  zp:zreg6wh
    lda  zp:zreg6wl + 1
    sta  zp:zreg6wh + 1

    lda  karl_lock
    bne  check$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

check$:
    lda  zp:zreg2b0
    beq  continue$

    ldx  zp:zreg2b2
    dex
    stx  zp:zreg0b0
    lda  #0x00
    sta  zp:zreg0b1

    sta  zp:zreg0b2
    sta  zp:zreg0b3

    ASLWMem  zp:zreg0wl
    ASLWMem  zp:zreg0wl

    LDQMem  zp:zreg7
    clc
    ADQMem  zp:zreg0
    STQMem  zp:zreg7

continue$:
    ldz  #0x00

    LDQIndDWZ  zp:zreg7
    STQMem  zp:zreg0

    lda  zp:zreg2b0
    beq  loop0$

loop2$:
    LDQMem  zp:zreg7

    sec
    SBQMem  zp:zvaltemp0

    JMP  proc$

loop0$:
    LDQMem  zp:zreg7
    clc
    ADQMem  zp:zvaltemp0

proc$:
    STQMem  zp:zreg7

    dec  zp:zreg2b2

    lda  zp:zreg6wh
    sta  karl_proxyptr
    lda  zp:zreg6wh + 1
    sta  karl_proxyptr + 1

    beq  next2$

    lda  #0x00
    sta  karl_errorno

    jsr  karlProxy

    lda  karl_errorno
    cmp  #ERROR_ABORT
    beq  finish$

next2$:
    lda  zp:zreg2b2
    bne  continue$

finish$:
    sei

    dec  karl_proccnt
    jsr  _karlProcObjLstLoad

    pla
    pla
    pla
    pla

    lda  karl_lock
    bne  exit$

  #ifndef DEBUG_NOYIELDIRQ
    cli
  #endif

exit$:
    rts

  .section rodata, rodata

karl_callstackidx:
    ;.repeat  FEATURE_CALLDEPTH, I
    ;.word  karl_callstack + (I * .sizeof(CALLCONTEXT))
    ;.endrepeat
    .word karl_callstack + (0 * sizeof_CALLCONTEXT)
    .word karl_callstack + (1 * sizeof_CALLCONTEXT)
    .word karl_callstack + (2 * sizeof_CALLCONTEXT)
    .word karl_callstack + (3 * sizeof_CALLCONTEXT)
    .word karl_callstack + (4 * sizeof_CALLCONTEXT)
    .word karl_callstack + (5 * sizeof_CALLCONTEXT)
    .word karl_callstack + (6 * sizeof_CALLCONTEXT)
    .word karl_callstack + (7 * sizeof_CALLCONTEXT)

karl_procstackidx:
    ;.repeat  FEATURE_CALLDEPTH, I
    ;.word  karl_procstack + (I * .sizeof(PROCCONTEXT))
    ;.endrepeat
    .word  karl_procstack + (0 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (1 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (2 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (3 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (4 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (5 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (6 * sizeof_PROCCONTEXT)
    .word  karl_procstack + (7 * sizeof_PROCCONTEXT)


karl_scrASCIIXlat:
  .byte  KEY_ASC_BSLASH, KEY_ASC_CARET, KEY_ASC_USCORE, KEY_ASC_BQUOTE
  .byte  KEY_ASC_OCRLYB, KEY_ASC_PIPE, KEY_ASC_CCRLYB, KEY_ASC_TILDE, 0x00
karl_scrASCIISub:
  .byte  0x4D, 0x71, 0x64, 0x4A ,0x55, 0x5D, 0x49, 0x1F, 0x00


  .section data, data


karl_temp0:
    .byte  0x00
karl_temp1:
    .byte  0x00
karl_temp2:
    .byte  0x00

karl_proxyptr:
    .word  0x0000

karl_errorno:
    .byte  0x00

karl_lock:
    .byte  0x00
karl_dirty:
    .byte  0x00
karl_changed:
    .byte  0x00


karl_callcnt:
    .byte  0x00

karl_callstack:
    .space  (sizeof_CALLCONTEXT * FEATURE_CALLDEPTH), 0

karl_proccnt:
    .byte  0x00

karl_procstack:
    .space  (sizeof_PROCCONTEXT * FEATURE_CALLDEPTH), 0
