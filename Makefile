VPATH = src

AS = as6502
CC = cc6502
LN = ln6502

CONFIG ?= default

GAME_ID = mm
DISK1_NAME = mm1.d81
DISK2_NAME = mm2.d81
DISK1_HEADER = maniac mansion
DISK2_HEADER = maniac mansion
DISK1_INDEX = m1
DISK2_INDEX = m2

SAVE_FILES = $(wildcard $(GAME_ID).sav.*)

CC_FLAGS       = --target=mega65 --code-model=plain -O2 -Werror --list-file=$(@:%.o=%.lst)
ifeq ($(CONFIG),debug)
	CC_FLAGS += -DDEBUG
endif

ifeq ($(CONFIG),debug_scripts)
	CC_FLAGS += -DDEBUG -DDEBUG_SCRIPTS
endif
CC_FLAGS_SPUTM = $(CC_FLAGS) --no-cross-call --strong-inline --inline-on-matching-custom-text-section --no-interprocedural-cross-jump
DEP_FLAGS      = -MMD -MP -MF$(@:%.o=%.d)
ASM_FLAGS      = --target=mega65 --list-file=$(@:%.o=%.lst)
LN_FLAGS       = --target=mega65 --verbose --rtattr printf=nofloat
LN_FLAGS_SETUP = $(LN_FLAGS) mega65-setup.scm --output-format=prg --list-file=setup-mega65.lst
LN_FLAGS_SPUTM = $(LN_FLAGS) mega65-sputm.scm --raw-multiple-memories --cstartup=sputm --rtattr exit=simplified --output-format=raw --list-file=sputm-mega65.lst

ETHLOAD   = etherload
M65FTP    = mega65_ftp
CC1541    = cc1541
XMEGA65   = /Applications/Xemu/xmega65.app/Contents/MacOS/xmega65

C_SRCS    = $(wildcard src/*.c)
ASM_SRCS  = $(wildcard src/*.s)
OBJS      = $(ASM_SRCS:src/%.s=obj/%_s.o) $(C_SRCS:src/%.c=obj/%.o)
DEPS      = $(OBJS:%.o=%.d)

SETUP_C_SRCS = $(wildcard src/setup/*.c)
SETUP_ASM_SRCS = $(wildcard src/setup/*.s)
SETUP_OBJS = $(SETUP_ASM_SRCS:src/setup/%.s=obj/setup/%_s.o) $(SETUP_C_SRCS:src/setup/%.c=obj/setup/%.o)
SETUP_DEPS = $(SETUP_OBJS:obj/setup/%.o=obj/setup/%.d)

export ETHLOAD_IP_PARAM

-include $(DEPS)
-include $(SETUP_DEPS)

.PHONY: all clean run debug_xemu doxygen

all: $(DISK1_NAME) $(DISK2_NAME)

run: $(DISK1_NAME) $(DISK2_NAME)
	$(M65FTP)  $(ETHLOAD_IP_PARAM) -e -c"put $(DISK1_NAME)"
	$(ETHLOAD) $(ETHLOAD_IP_PARAM) -m $(DISK1_NAME) -r autoboot.raw

debug_xemu: $(DISK1_NAME) $(DISK2_NAME)
	@echo "--------------------------------------------------"
	@echo "Starting Xemu..."
	@echo "Make sure a tmux session named 'megasputm-xemu' is running"
	@echo "Use 'xemu_tmux_session.sh' to create one"
	@echo "--------------------------------------------------"
	tmux send-keys -t megasputm-xemu "$(XMEGA65) -uartmon :4510 -8 $(DISK1_NAME) -besure -curskeyjoy -videostd 0" C-m

obj/setup/%_s.o: src/setup/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/setup/%.o: src/setup/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $(DEP_FLAGS) -c $< -o $@
obj/%_s.o: src/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASM_FLAGS) -o $@ $<

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS_SPUTM) $(DEP_FLAGS) -c $< -o $@

setup.prg: $(SETUP_OBJS)
	$(LN) $(LN_FLAGS_SETUP) -o setup.prg $(SETUP_OBJS)

autoboot.raw: $(OBJS) mega65-sputm.scm
	$(LN) $(LN_FLAGS_SPUTM) -o $@ $(filter-out mega65-sputm.scm,$^)

$(DISK1_NAME): autoboot.raw setup.prg autoboot.c65.bas $(SAVE_FILES)
	@echo "Creating  $(DISK1_NAME) disk image"; \
	$(CC1541) -q -n "$(DISK1_HEADER)" -i "$(DISK1_INDEX)#a03d" -f autoboot.c65 -w autoboot.c65.bas -f boot -w autoboot.raw -f setup -w setup.prg -f m01 -w script.raw -f m02 -w main.raw -f m03 -w m0-3.raw -f m10 -w m1-0.raw -f m12 -w m1-2.raw -f m13 -w m1-3.raw -f m14 -w m1-4.raw -f mc0 -w mc-0.raw $(DISK1_NAME); \
	for file in gamedata/$(GAME_ID)/disk1/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(CC1541) -q -f $$lowercasefile -w $$file $(DISK1_NAME); \
		elif [ "$$ext" = "lfl" ]; then \
			$(CC1541) -q -f $$(basename $$file) -w $$file $(DISK1_NAME); \
		fi; \
	done; \
	echo "Copying save game files to disk image..."; \
	for file in $(SAVE_FILES); do \
		if [ -f "$$file" ]; then \
			echo "Adding $$file to $(DISK1_NAME)..."; \
			@$(C1541) -attach $(DISK1_NAME) -write $$file $$(basename $$file),s; \
		fi \
	done

$(DISK2_NAME):
	@echo "Creating $(DISK2_NAME) disk image"; \
	$(CC1541) -q -n "$(DISK2_HEADER)" -i "$(DISK2_INDEX)#a03d" $(DISK2_NAME); \
	for file in gamedata/$(GAME_ID)/disk2/*; do \
		ext=$${file##*.}; \
		lowercasefile=$$(basename $$file | tr '[:upper:]' '[:lower:]'); \
		if [ "$$ext" = "LFL" ]; then \
			$(CC1541) -q -f $$lowercasefile -w $$file $(DISK2_NAME); \
		elif [ "$$ext" = "lfl" ]; then \
			$(CC1541) -q -f $$(basename $$file) -w $$file $(DISK2_NAME); \
		fi; \
	done

doxygen:
	doxygen Doxyfile

clean:
	-rm -rf obj
	-rm *.prg *.raw *.d *.lst *.d81
