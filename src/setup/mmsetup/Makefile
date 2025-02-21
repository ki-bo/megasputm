VPATH = src

AS = as6502
CC = cc6502
LN = ln6502

CONFIG ?= default

CC_FLAGS  = -l --target=mega65 --code-model=plain -O2 -Werror --no-cross-call --strong-inline --inline-on-matching-custom-text-section --no-interprocedural-cross-jump --list-file=$(@:%.o=%.lst)
DEP_FLAGS = -MMD -MP
ASM_FLAGS = --target=mega65 --list-file=$(@:%.o=%.lst)
LN_FLAGS  = --target=mega65 mega65-tilda.scm --verbose --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat --output-format=prg --list-file=mmsetup.lst

ETHLOAD   = etherload
M65FTP    = mega65_ftp
C1541     = c1541
XMEGA65   = /Applications/Xemu/xmega65.app/Contents/MacOS/xmega65

C_SRCS    = $(wildcard *.c)
ASM_SRCS  = $(wildcard *.s)
OBJS      = $(ASM_SRCS:%.s=obj/%_s.o) $(C_SRCS:%.c=obj/%.o)
DEPS      = $(OBJS:%.o=%.d)

ifeq ($(CONFIG),debug)
	CC_FLAGS += -DDEBUG
endif

ifeq ($(CONFIG),debug_scripts)
	CC_FLAGS += -DDEBUG -DDEBUG_SCRIPTS
endif

export ETHLOAD_IP_PARAM

-include $(DEPS)

.PHONY: all clean run debug_xemu doxygen

all: mmsetup.prg

obj/%_s.o: %.s
#	@mkdir -p obj
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/%.o: %.c
#	@mkdir -p obj
	$(CC) $(CC_FLAGS) $(DEP_FLAGS) -c $< -o $@ -MFobj/$*.d

mmsetup.prg: $(OBJS) _karljr_types.inc _jude_types.inc _jude_widgets.inc mega65-tilda.scm
	$(LN) $(LN_FLAGS) -o $@ $(filter-out _karljr_types.inc _jude_types.inc _jude_widgets.inc mega65-tilda.scm,$^)

