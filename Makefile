VPATH = src

ETHLOAD = /Users/robert/bin/etherload.osx
M65FTP = /Users/robert/bin/mega65_ftp.osx


.phony: all clean run debug_xemu
run: mm.d81
	$(M65FTP) -e -c"put mm.d81"
	$(ETHLOAD) -m mm.d81 -r autoboot.raw

debug_xemu: mm.d81
	/Applications/Xemu/xmega65.app/Contents/MacOS/xmega65 -uartmon :4510 -8 mm.d81 -besure

# Common source files
ASM_SRCS = startup.s
C_SRCS = main.c         \
         diskio.c       \
		 dma.c          \
		 gfx.c          \
		 init.c         \
		 map.c          \
		 resource.c     \
		 script.c       \
		 util.c

# Object files
OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=obj/%-debug.o) $(C_SRCS:%.c=obj/%-debug.o)

obj/%.o: %.s
	@mkdir -p $(@D)
	as6502 --target=mega65 --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	@mkdir -p $(@D)
	cc6502 --target=mega65 --code-model=plain -O2 --no-cross-call --strong-inline --inline-on-matching-custom-text-section --list-file=$(@:%.o=%.lst) -o $@ $<

runtime.raw:  $(OBJS) mega65-mm.scm
	ln6502 --target=mega65 mega65-mm.scm --verbose --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat -o $@ $(filter-out mega65-mm.scm,$^) --output-format=raw --list-file=mm-mega65.lst 

mm.elf: $(OBJS) mega65-mm.scm
	ln6502 --target=mega65 mega65-mm.scm --debug --verbose --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat -o $@ $(filter-out mega65-mm.scm,$^) --list-file=mm-debug.lst --semi-hosted

mm.d81: runtime.raw
	cp gamedata/MM.D81 mm.d81
	c1541 -attach mm.d81 -write autoboot.raw autoboot.c65 -write runtime.raw m00 -write main.raw m01 -write m1-1.raw m11


clean:
	-rm -rf obj
	-rm mm.elf mmprg.raw mm-mega65.lst mm-debug.lst
