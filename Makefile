VPATH = src

AS = as6502
CC = cc6502
LN = ln6502

CC_FLAGS  = --target=mega65 --code-model=plain -O2 --no-cross-call --strong-inline --inline-on-matching-custom-text-section --list-file=$(@:%.o=%.lst)
DEP_FLAGS = -MMD -MP
ASM_FLAGS = --target=mega65 --list-file=$(@:%.o=%.lst)
LN_FLAGS  = --target=mega65 mega65-mm.scm --verbose --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat --output-format=raw --list-file=mm-mega65.lst

ETHLOAD   = etherload.osx
M65FTP    = mega65_ftp.osx
C1541     = c1541
XMEGA65   = /Applications/Xemu/xmega65.app/Contents/MacOS/xmega65

C_SRCS    = $(wildcard src/*.c)
ASM_SRCS  = $(wildcard src/*.s)
OBJS      = $(ASM_SRCS:src/%.s=obj/%.o) $(C_SRCS:src/%.c=obj/%.o)
DEPS      = $(OBJS:%.o=%.d)

-include $(DEPS)

.PHONY: all clean run debug_xemu

all: mm.d81

run: mm.d81
	$(M65FTP) -e -c"put mm.d81"
	$(ETHLOAD) -m mm.d81 -r autoboot.raw

debug_xemu: mm.d81
	$(XMEGA65) -uartmon :4510 -8 mm.d81 -besure

obj/%.o: %.s
	@mkdir -p obj
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/%.o: %.c
	@mkdir -p obj
	$(CC) $(CC_FLAGS) $(DEP_FLAGS) -c $< -o $@

runtime.raw:  $(OBJS) mega65-mm.scm
	$(LN) $(LN_FLAGS) -o $@ $(filter-out mega65-mm.scm,$^)

mm.d81: runtime.raw
	cp gamedata/MM.D81 mm.d81
	$(C1541) -attach mm.d81 -write autoboot.raw autoboot.c65 -write runtime.raw m00 -write main.raw m01 -write m1-1.raw m11

clean:
	-rm -rf obj
	-rm *.raw *.d mm-mega65.lst mm.d81
