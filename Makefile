VPATH = src

AS = as6502
CC = cc6502
LN = ln6502

CONFIG ?= default

SAVE_FILES = $(wildcard mm.sav.*)

CC_FLAGS  = --target=mega65 --code-model=plain -O2 -Werror --no-cross-call --strong-inline --inline-on-matching-custom-text-section --no-interprocedural-cross-jump --list-file=$(@:%.o=%.lst)
DEP_FLAGS = -MMD -MP
ASM_FLAGS = --target=mega65 --list-file=$(@:%.o=%.lst)
LN_FLAGS  = --target=mega65 mega65-mm.scm --verbose --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --rtattr printf=nofloat --output-format=raw --list-file=mm-mega65.lst

ETHLOAD   = etherload.osx
M65FTP    = mega65_ftp.osx
C1541     = c1541
XMEGA65   = /Applications/Xemu/xmega65.app/Contents/MacOS/xmega65

C_SRCS    = $(wildcard src/*.c)
ASM_SRCS  = $(wildcard src/*.s)
OBJS      = $(ASM_SRCS:src/%.s=obj/%_s.o) $(C_SRCS:src/%.c=obj/%.o)
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

all: mm1.d81 mm2.d81

run: mm1.d81 mm2.d81
	$(M65FTP)  $(ETHLOAD_IP_PARAM) -e -c"put mm1.d81"
	$(ETHLOAD) $(ETHLOAD_IP_PARAM) -m mm1.d81 -r runtime.raw

debug_xemu: mm1.d81
	@echo "--------------------------------------------------"
	@echo "Starting Xemu..."
	@echo "Make sure a tmux session named 'mmxemu' is running"
	@echo "Use 'xemu_tmux_session.sh' to create one"
	@echo "--------------------------------------------------"
	tmux send-keys -t mmxemu "$(XMEGA65) -uartmon :4510 -8 mm1.d81 -besure -curskeyjoy -videostd 0" C-m

obj/%_s.o: %.s
	@mkdir -p obj
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/%.o: %.c
	@mkdir -p obj
	$(CC) $(CC_FLAGS) $(DEP_FLAGS) -c $< -o $@ -MFobj/$*.d

runtime.raw: $(OBJS) mega65-mm.scm
	$(LN) $(LN_FLAGS) -o $@ $(filter-out mega65-mm.scm,$^)

mm1.d81: runtime.raw $(SAVE_FILES)
	echo "creating  mm2.d81 disk image"; \
	$(C1541) -format "maniac mansion,m1" d81 mm1.d81; \
	$(C1541) -attach mm1.d81 -write runtime.raw autoboot.c65 -write script.raw m01 -write main.raw m02 -write m0-3.raw m03 -write m1-0.raw m10 -write m1-2.raw m12 -write m1-3.raw m13 -write mc-0.raw mc0; \
	for file in gamedata/disk1/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(C1541) -attach mm1.d81 -write $$file $$lowercasefile; \
		elif [ "$$ext" = "lfl" ]; then \
			$(C1541) -attach mm1.d81 -write $$file $$(basename $$file); \
		fi; \
	done; \

mm2.d81:
	echo "creating mm2.d81 disk image"; \
	$(C1541) -format "maniac mansion,m2" d81 mm2.d81; \
	for file in gamedata/disk2/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(C1541) -attach mm2.d81 -write $$file $$lowercasefile; \
		elif [ "$$ext" = "lfl" ]; then \
			$(C1541) -attach mm2.d81 -write $$file $$(basename $$file); \
		fi; \
	done

doxygen:
	doxygen Doxyfile

clean:
	-rm -rf obj
	-rm *.raw *.d mm-mega65.lst mm1.d81 mm2.d81
