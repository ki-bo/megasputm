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
DEP_FLAGS      = -MMD -MP
ASM_FLAGS      = --target=mega65 --list-file=$(@:%.o=%.lst)
LN_FLAGS       = --target=mega65 --verbose --rtattr printf=nofloat
LN_FLAGS_SETUP = $(LN_FLAGS) mega65-plain.scm --output-format=prg --list-file=setup-mega65.lst
LN_FLAGS_MM    = $(LN_FLAGS) mega65-mm.scm --no-tree-shaking --raw-multiple-memories --cstartup=mm --rtattr exit=simplified --output-format=raw --list-file=mm-mega65.lst

ETHLOAD   = etherload
M65FTP    = mega65_ftp
C1541     = c1541
XMEGA65   = /Applications/Xemu/xmega65.app/Contents/MacOS/xmega65

C_SRCS    = $(wildcard src/*.c)
ASM_SRCS  = $(wildcard src/*.s)
OBJS      = $(ASM_SRCS:src/%.s=obj/%_s.o) $(C_SRCS:src/%.c=obj/%.o)
DEPS      = $(OBJS:%.o=%.d)

# Setup tool sources and objects
SETUP_C_SRCS = $(wildcard src/setup/*.c)
SETUP_ASM_SRCS = $(wildcard src/setup/*.s)
SETUP_OBJS = $(SETUP_ASM_SRCS:src/setup/%.s=obj/setup/%_s.o) $(SETUP_C_SRCS:src/setup/%.c=obj/setup/%.o)
SETUP_DEPS = $(SETUP_OBJS:obj/setup/%.o=obj/setup/%.d)

export ETHLOAD_IP_PARAM

-include $(DEPS)
-include $(SETUP_DEPS)

.PHONY: all clean run debug_xemu doxygen

all: mm1.d81 mm2.d81 setup.prg

run: mm1.d81 mm2.d81
	$(M65FTP)  $(ETHLOAD_IP_PARAM) -e -c"put mm1.d81"
	$(ETHLOAD) $(ETHLOAD_IP_PARAM) -m mm1.d81 -r runtime.raw

debug_xemu: mm1.d81 mm2.d81
	@echo "--------------------------------------------------"
	@echo "Starting Xemu..."
	@echo "Make sure a tmux session named 'mmxemu' is running"
	@echo "Use 'xemu_tmux_session.sh' to create one"
	@echo "--------------------------------------------------"
	tmux send-keys -t mmxemu "$(XMEGA65) -uartmon :4510 -8 mm1.d81 -besure -curskeyjoy -videostd 0" C-m

# Rules for compiling C and assembly files
obj/setup/%_s.o: src/setup/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/setup/%.o: src/setup/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $(DEP_FLAGS) -c $< -o $@ -MFobj/$*.d

obj/%_s.o: src/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS_MM) $(DEP_FLAGS) -c $< -o $@ -MFobj/$*.d

# Rule for building the setup target
setup.prg: $(SETUP_OBJS)
	$(LN) $(LN_FLAGS_SETUP) -o setup.prg $(SETUP_OBJS)

# Rule for building the runtime.raw target
runtime.raw: $(OBJS) mega65-mm.scm
	$(LN) $(LN_FLAGS_MM) -o $@ $(filter-out mega65-mm.scm,$^)

# Rule for creating the mm1.d81 disk image
mm1.d81: runtime.raw setup.prg $(SAVE_FILES)
	echo "creating  mm1.d81 disk image"; \
	$(C1541) -format "maniac mansion,m1" d81 mm1.d81; \
	$(C1541) -attach mm1.d81 -write runtime.raw autoboot.c65 -write setup.prg setup -write script.raw m01 -write main.raw m02 -write m0-3.raw m03 -write m1-0.raw m10 -write m1-2.raw m12 -write m1-3.raw m13 -write mc-0.raw mc0; \
	for file in gamedata/disk1/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(C1541) -attach mm1.d81 -write $$file $$lowercasefile; \
		elif [ "$$ext" = "lfl" ]; then \
			$(C1541) -attach mm1.d81 -write $$file $$(basename $$file); \
		fi; \
	done; \
	echo "Copying save game files to disk image..."
	for file in $(SAVE_FILES); do \
		if [ -f "$$file" ]; then \
			echo "Adding $$file to mm1.d81..."; \
			$(C1541) -attach mm1.d81 -write $$file $$(basename $$file),s; \
		fi \
	done

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
	rm -f obj/*.o obj/*.lst obj/*.d
	rm -f obj/setup/*.o obj/setup/*.lst obj/setup/*.d
	rm -f mm1.d81 mm2.d81 setup.prg
