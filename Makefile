VPATH = src

AS = as6502
CC = cc6502
LN = ln6502

CONFIG ?= default

SAVE_FILES = $(wildcard savegames/mm.sav.*)

CC_FLAGS       = --target=mega65 --code-model=plain -O2 -Werror --list-file=$(@:%.o=%.lst)
ifeq ($(CONFIG),debug)
	CC_FLAGS += -DDEBUG
endif

ifeq ($(CONFIG),debug_scripts)
	CC_FLAGS += -DDEBUG -DDEBUG_SCRIPTS
endif
CC_FLAGS_MM    = $(CC_FLAGS) --no-cross-call --strong-inline --inline-on-matching-custom-text-section --no-interprocedural-cross-jump
DEP_FLAGS      = -MMD -MP -MF$(@:%.o=%.d)
ASM_FLAGS      = --target=mega65 --list-file=$(@:%.o=%.lst)
LN_FLAGS       = --target=mega65 --verbose --rtattr printf=nofloat
LN_FLAGS_SETUP = $(LN_FLAGS) mega65-mmsetup.scm --output-format=prg --list-file=mmsetup-mega65.lst
LN_FLAGS_MM    = $(LN_FLAGS) mega65-mm.scm --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --output-format=raw --list-file=mm-mega65.lst

ETHLOAD   = etherload
M65FTP    = mega65_ftp
CC1541    = cc1541
XMEGA65   = /Applications/Xemu/xmega65.app/Contents/MacOS/xmega65

# megasputm engine sources and objects
C_SRCS    = $(wildcard src/*.c)
ASM_SRCS  = $(wildcard src/*.s)
OBJS      = $(ASM_SRCS:src/%.s=obj/%_s.o) $(C_SRCS:src/%.c=obj/%.o)
DEPS      = $(OBJS:%.o=%.d)

# Setup tool sources and objects
SETUP_C_SRCS = $(wildcard src/mmsetup/*.c)
SETUP_ASM_SRCS = $(wildcard src/mmsetup/*.s)
SETUP_OBJS = $(SETUP_ASM_SRCS:src/mmsetup/%.s=obj/mmsetup/%_s.o) $(SETUP_C_SRCS:src/mmsetup/%.c=obj/mmsetup/%.o)
SETUP_DEPS = $(SETUP_OBJS:obj/mmsetup/%.o=obj/mmsetup/%.d)

export ETHLOAD_IP_PARAM

-include $(DEPS)
-include $(SETUP_DEPS)

.PHONY: all clean run debug_xemu doxygen

all: mm1.d81 mm2.d81

run: mm1.d81 mm2.d81
	$(M65FTP)  $(ETHLOAD_IP_PARAM) -e -c"put mm1.d81"
	$(ETHLOAD) $(ETHLOAD_IP_PARAM) -m mm1.d81 -r autoboot.raw

debug_xemu: mm1.d81 mm2.d81
	@echo "--------------------------------------------------"
	@echo "Starting Xemu..."
	@echo "Make sure a tmux session named 'mmxemu' is running"
	@echo "Use 'xemu_tmux_session.sh' to create one"
	@echo "--------------------------------------------------"
	tmux send-keys -t mmxemu "$(XMEGA65) -uartmon :4510 -8 mm1.d81 -besure -curskeyjoy -videostd 0" C-m

# Rules for compiling C and assembly files
obj/mmsetup/%_s.o: src/mmsetup/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/mmsetup/%.o: src/mmsetup/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $(DEP_FLAGS) -c $< -o $@
obj/%_s.o: src/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS_MM) $(DEP_FLAGS) -c $< -o $@

# Rule for building the mmsetup target
mmsetup.prg: $(SETUP_OBJS)
	$(LN) $(LN_FLAGS_SETUP) -o mmsetup.prg $(SETUP_OBJS)

# Rule for building the autoboot.raw target
autoboot.raw: $(OBJS) mega65-mm.scm
	$(LN) $(LN_FLAGS_MM) -o $@ $(filter-out mega65-mm.scm,$^)

# Rule for creating the mm1.d81 disk image
mm1.d81: autoboot.raw mmsetup.prg autoboot.c65.bas $(SAVE_FILES)
	@echo "Creating  mm1.d81 disk image"; \
	$(CC1541) -q -n "maniac mansion" -i "m1#a03d" -f autoboot.c65 -w autoboot.c65.bas -f boot -w autoboot.raw -f mmsetup -w mmsetup.prg -f m01 -w script.raw -f m02 -w main.raw -f m03 -w m0-3.raw -f m10 -w m1-0.raw -f m12 -w m1-2.raw -f m13 -w m1-3.raw -f m14 -w m1-4.raw -f mc0 -w mc-0.raw mm1.d81; \
	for file in gamedata/disk1/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(CC1541) -q -f $$lowercasefile -w $$file mm1.d81; \
		elif [ "$$ext" = "lfl" ]; then \
			$(CC1541) -q -f $$(basename $$file) -w $$file mm1.d81; \
		fi; \
	done; \
	echo "Copying save game files to disk image..."; \
	for file in $(SAVE_FILES); do \
		if [ -f "$$file" ]; then \
			echo "Adding $$file to mm1.d81..."; \
			@$(C1541) -attach mm1.d81 -write $$file $$(basename $$file),s; \
		fi \
	done

mm2.d81:
	@echo "Creating mm2.d81 disk image"; \
	$(CC1541) -q -n "maniac mansion" -i "m2#a03d" mm2.d81; \
	for file in gamedata/disk2/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(CC1541) -q -f $$lowercasefile -w $$file mm2.d81; \
		elif [ "$$ext" = "lfl" ]; then \
			$(CC1541) -q -f $$(basename $$file) -w $$file mm2.d81; \
		fi; \
	done

doxygen:
	doxygen Doxyfile

clean:
	-rm -rf obj
	-rm *.prg *.raw *.d *.lst mm1.d81 mm2.d81
