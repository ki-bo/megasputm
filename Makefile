VPATH = src

# Common source files
ASM_SRCS = startup.s
C_SRCS = main.c   \
         diskio.c \
		 dma.c    \
		 gfx.c    \
		 util.c

# Object files
OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=obj/%-debug.o) $(C_SRCS:%.c=obj/%-debug.o)

obj/%.o: %.s
	@mkdir -p $(@D)
	as6502 --target=mega65 --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	@mkdir -p $(@D)
	cc6502 --target=mega65 --code-model=plain -O2 --no-cross-call --strong-inline --debug --list-file=$(@:%.o=%.lst) -o $@ $<

mmprg.raw:  $(OBJS) mega65-mm.scm
	ln6502 --target=mega65 mega65-mm.scm --verbose --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat -o $@ $(filter-out mega65-mm.scm,$^) --output-format=raw --list-file=mm-mega65.lst 

mm.elf: $(OBJS) mega65-mm.scm
	ln6502 --target=mega65 mega65-mm.scm --debug --verbose --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat -o $@ $(filter-out mega65-mm.scm,$^) --list-file=mm-debug.lst --semi-hosted

clean:
	-rm -rf obj
	-rm mm.elf mmprg.raw mm-mega65.lst mm-debug.lst
