all:
	$(MAKE) -f lpg_module/lpg_module.mk

clean:
	rm -rf build/

flash-lpg_module: lpg_module/lpg_module.mk
	$(MAKE) -f $^ flash

dfu-lpg_module: lpg_module/lpg_module.mk
	$(MAKE) -f $^ dfu

.PHONY: all clean

arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz:
	wget https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz

install-toolchain: arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz
	tar xf arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz
