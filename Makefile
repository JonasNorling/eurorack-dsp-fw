TOOLCHAIN_DIR=arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/
CC=$(TOOLCHAIN_DIR)/arm-none-eabi-gcc
SIZE=$(TOOLCHAIN_DIR)/arm-none-eabi-size

INCLUDE_DIR=include
CPPFLAGS=\
    -DSTM32G474xx \
	-I$(INCLUDE_DIR) \
	-Istm32-cube/Drivers/CMSIS/Include \
	-Istm32-cube/Drivers/STM32G4xx_HAL_Driver/Inc \
	-Istm32-cube/Drivers/CMSIS/Device/ST/STM32G4xx/Include \
	-Istm32-cube/Drivers/BSP/STM32G474E-EVAL

CFLAGS=-mcpu=cortex-m4 -mfloat-abi=hard -Wall -Wextra -Werror -g -Og
LDFLAGS=-Wl,-Tsrc/STM32G474CCTX_FLASH.ld --specs=nano.specs --specs=nosys.specs

BUILDDIR=build

SRCS += src/main.c
SRCS += src/serial_log.c
SRCS += src/audio_codec.c
SRCS += src/system_stm32g4xx.c
SRCS += src/stm32g4xx_it.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_usart.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_usart_ex.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_sai.c
SRCS += src/startup_stm32g474retx.s

OBJS := $(SRCS:%.c=$(BUILDDIR)/%.o)

all: $(BUILDDIR)/application.elf

clean:
	rm -rf build

$(BUILDDIR)/%.o: %.c
	@echo CC $@
	@mkdir -p $(dir $@)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILDDIR)/application.elf: $(OBJS)
	@echo LD $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

flash: flash-stm32cubeprogrammer
flash-stm32cubeprogrammer: $(BUILDDIR)/application.elf
	~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
		--connect port=swd reset=HWrst --download $^ --start

flash-openocd: $(BUILDDIR)/application.elf
	openocd -f interface/stlink-v2.cfg \
		-f board/st_nucleo_g4.cfg \
		-c "init" -c "reset init" \
		-c "flash write_image erase $<" \
		-c "reset" \
		-c "shutdown"

gdbserver:
	openocd \
		-f board/st_nucleo_g4.cfg \
		-c "tcl_port 6333" -c "telnet_port 4444" -c "gdb_port 3333"

gdb: $(BUILDDIR)/application.elf
	$(TOOLCHAIN_DIR)/arm-none-eabi-gdb $^ -ex "target remote :3333"

arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz:
	wget https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz

install-toolchain: arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz
	tar xf arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz
