CC=~/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
SIZE=~/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-size

INCLUDE_DIR=include
CPPFLAGS=\
    -DSTM32G474xx \
	-I$(INCLUDE_DIR) \
	-Istm32-cube/Drivers/CMSIS/Include \
	-Istm32-cube/Drivers/STM32G4xx_HAL_Driver/Inc \
	-Istm32-cube/Drivers/CMSIS/Device/ST/STM32G4xx/Include \
	-Istm32-cube/Drivers/BSP/STM32G474E-EVAL

CFLAGS=-mcpu=cortex-m4 -Wall -Wextra -Werror
LDFLAGS=-Wl,-Tstm32-cube/Projects/STM32G474E-EVAL/Templates/STM32CubeIDE/STM32G474RETX_FLASH.ld

BUILDDIR=build

SRCS += src/main.c
SRCS += src/system_stm32g4xx.c
SRCS += src/stm32g4xx_it.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
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
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(SIZE) $@
