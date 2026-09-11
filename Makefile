BINARY		= stm32_betty
OUT_DIR     = obj
PREFIX	  ?= arm-none-eabi
SIZE  = $(PREFIX)-size
CC		= $(PREFIX)-gcc
CPP	= $(PREFIX)-g++
LD		= $(PREFIX)-gcc
OBJCOPY		= $(PREFIX)-objcopy
MKDIR_P     = mkdir -p

CFLAGS		= -Os -Wall -Wextra -Ilibopeninv/include -Iinclude/ -Ilibopencm3/include \
              -fno-common -fno-builtin -DSTM32F1 -DMAX_USER_MESSAGES=30 \
				  -mcpu=cortex-m3 -mthumb -std=gnu99 -ffunction-sections -fdata-sections -ggdb3
CPPFLAGS    = -Os -Wall -Wextra -Ilibopeninv/include -Iinclude/ -Ilibopencm3/include \
              -fno-common -std=c++17 -DSTM32F1 -DMAX_USER_MESSAGES=30  \
				  -ffunction-sections -fdata-sections -fno-builtin -fno-rtti -fno-exceptions \
              -fno-unwind-tables -mcpu=cortex-m3 -mthumb -ggdb3
LDSCRIPT	= $(BINARY).ld
LDFLAGS  = -Llibopencm3/lib -T$(LDSCRIPT) -march=armv7 -nostartfiles -Wl,--gc-sections,-Map,linker.map

OBJSL		= $(BINARY).o hwinit.o bmw_crc.o stm32scheduler.o params.o terminal.o terminal_prj.o \
           my_string.o digio.o my_fp.o printf.o anain.o param_save.o errormessage.o \
           stm32_can.o canhardware.o terminalcommands.o canmap.o

OBJS     = $(patsubst %.o,$(OUT_DIR)/%.o, $(OBJSL))
vpath %.c src/ libopeninv/src/
vpath %.cpp src/ libopeninv/src/

comma := ,
link_command := -Wl$(comma)
try-run = $(shell set -e; if ($(1)) >/dev/null 2>&1; then echo "$(2)"; else echo "$(3)"; fi)
ld-option = $(call try-run, $(PREFIX)-ld $(1) -v,$(link_command)$(1))
LDFLAGS	+= $(call ld-option,--no-warn-rwx-segments)
LDFLAGS  = -Llibopencm3/lib -T$(LDSCRIPT) -march=armv7 -nostartfiles \
           -Wl,--gc-sections,-Map,linker.map,--undefined=vector_table

all: directories images
Debug:images
Release: images
cleanDebug:clean
images: get-deps $(BINARY)
	@printf "  OBJCOPY $(BINARY).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(BINARY) $(BINARY).bin
	@printf "  OBJCOPY $(BINARY).hex\n"
	$(Q)$(OBJCOPY) -Oihex $(BINARY) $(BINARY).hex
	$(Q)$(SIZE) $(BINARY)

directories: ${OUT_DIR}
${OUT_DIR}:
	$(Q)${MKDIR_P} ${OUT_DIR}

$(BINARY): $(OBJS) $(LDSCRIPT)
	@printf "  LD      $(@)\n"
	$(Q)$(LD) $(LDFLAGS) -o $(BINARY) $(OBJS) -lopencm3_stm32f1 -lm

$(OUT_DIR)/%.o: %.c Makefile
	@printf "  CC      $(@)\n"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -o $@ -c $<

$(OUT_DIR)/%.o: %.cpp Makefile
	@printf "  CPP     $(@)\n"
	$(Q)$(CPP) $(CPPFLAGS) -MMD -MP -o $@ -c $<

DEP = $(OBJS:%.o=%.d)
-include $(DEP)

clean:
	$(Q)rm -rf ${OUT_DIR} $(BINARY) $(BINARY).bin $(BINARY).hex linker.map

.PHONY: directories get-deps images clean

get-deps:
ifneq ($(shell test -s libopencm3/lib/libopencm3_stm32f1.a && echo -n yes),yes)
	@printf "  GIT DEPS\n"
	$(Q)if [ ! -f libopencm3/Makefile ]; then \
		git submodule update --init --recursive || true; \
	fi
	$(Q)if [ ! -f libopencm3/Makefile ]; then \
		git clone --depth 1 https://github.com/jsphuebner/libopencm3.git libopencm3; \
	fi
	$(Q)if [ ! -f libopeninv/include/params.h ]; then \
		git clone --depth 1 https://github.com/jsphuebner/libopeninv.git libopeninv; \
	fi
	@printf "  MAKE libopencm3\n"
	$(Q)${MAKE} -C libopencm3 TARGETS=stm32/f1
endif

ifneq ($(V),1)
Q := @
endif
