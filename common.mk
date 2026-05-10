TOOLCHAIN_DIR=arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/
CC=$(TOOLCHAIN_DIR)/arm-none-eabi-gcc
SIZE=$(TOOLCHAIN_DIR)/arm-none-eabi-size
OBJCOPY=$(TOOLCHAIN_DIR)/arm-none-eabi-objcopy

CPPFLAGS=\
    -MP -MMD \
    -DSTM32G474xx \
	-I. \
	-Iinclude -Iinclude/system \
	-Istm32-cube/Drivers/CMSIS/Include \
	-Istm32-cube/Drivers/STM32G4xx_HAL_Driver/Inc \
	-Istm32-cube/Drivers/CMSIS/Device/ST/STM32G4xx/Include \
	-Istm32-cube/Drivers/BSP/STM32G474E-EVAL

CFLAGS=-mcpu=cortex-m4 -mfloat-abi=hard -mthumb -Wall -Wextra -Werror -Wshadow -g -Og
LDFLAGS=-Wl,-Tsrc/system/STM32G474CCTX_FLASH.ld --specs=nano.specs --specs=nosys.specs

OBJS := $(SRCS:%.c=$(BUILDDIR)/%.o) $(ASM_SRCS:%.s=$(BUILDDIR)/%.o)
DEP_FILES = $(SRCS:%.c=$(BUILDDIR)/%.d)

clean:
	rm -rf build

.PHONY: all clean

src/dsp/lut_data.c:
	tools/make_lut.py $@

-include $(DEP_FILES)

$(BUILDDIR)/%.o: %.c
	@echo CC $@
	@mkdir -p $(dir $@)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILDDIR)/%.o: %.s
	@echo CC $@
	@mkdir -p $(dir $@)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(MAIN_ELF): $(OBJS)
	@echo LD $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ -lm
	$(SIZE) $@

flash: flash-stm32cubeprogrammer
flash-stm32cubeprogrammer: $(MAIN_ELF)
	~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
		--connect port=swd reset=HWrst --download $^ --start

flash-openocd: $(MAIN_ELF)
	openocd -f interface/stlink-v2.cfg \
		-f board/st_nucleo_g4.cfg \
		-c "init" -c "reset init" \
		-c "flash write_image erase $<" \
		-c "reset" \
		-c "shutdown"

dfu: $(MAIN_ELF:.elf=.bin-dfu)
	dfu-util -a 0 -s 0x08000000 -D $<

$(BUILDDIR)/%.bin-dfu: $(BUILDDIR)/%.bin
	cp $< $@
	dfu-suffix -a $@ -v 0483 -p df11

%.bin: %.elf
	@echo flattening $@
	@$(OBJCOPY) -Obinary $< $@

gdbserver:
	openocd \
		-f board/st_nucleo_g4.cfg \
		-c "tcl_port 6333" -c "telnet_port 4444" -c "gdb_port 3333"

gdb: $(MAIN_ELF)
	$(TOOLCHAIN_DIR)/arm-none-eabi-gdb $^ -ex "target remote :3333"
