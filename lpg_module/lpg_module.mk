SRCS += lpg_module/main.c
SRCS += lpg_module/gpio.c
SRCS += src/system/serial_log.c
SRCS += src/system/audio_codec.c
SRCS += src/system/analog_in.c
SRCS += src/system/analog_out.c
SRCS += lpg_module/lpg_main.c
SRCS += lpg_module/lfo_main.c
SRCS += src/dsp/biquad.c
SRCS += src/dsp/lut_data.c

SRCS += src/system/system_stm32g4xx.c
SRCS += src/system/stm32g4xx_it.c

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
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_adc.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_adc_ex.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dac.c
SRCS += stm32-cube/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dac_ex.c

ASM_SRCS += src/system/startup_stm32g474retx.s

OBJS := $(SRCS:%.c=$(BUILDDIR)/%.o) $(ASM_SRCS:%.s=$(BUILDDIR)/%.o)
DEP_FILES = $(SRCS:%.c=$(BUILDDIR)/%.d)
BUILDDIR=build/lpg_module
MAIN_ELF=$(BUILDDIR)/lpg_module.elf

all: $(MAIN_ELF)

include common.mk
