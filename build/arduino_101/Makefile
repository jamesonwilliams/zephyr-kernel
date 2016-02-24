ZEPHYR_GCC_VARIANT ?= zephyr
ZEPHYR_SDK_INSTALL_DIR ?= /opt/zephyr-sdk

current_dir = $(shell pwd)
O ?= $(current_dir)/out

arc_app := $(ZEPHYR_BASE)/samples/nanokernel/apps/hello_world
x86_app := $(ZEPHYR_BASE)/samples/microkernel/apps/hello_world
ble_app := $(ZEPHYR_BASE)/samples/bluetooth/peripheral

image: x86_image arc_image ble_image 

ble_image:
	mkdir -p $(O)/ble
	make -C $(ble_app) ARCH=arm O=$(O)/ble BOARD=basic_cortex_m3 

x86_image:
	mkdir -p $(O)/x86
	make -C $(x86_app)/ ARCH=x86 O=$(O)/x86 BOARD=arduino_101

arc_image:
	mkdir -p $(O)/arc
	make -C $(arc_app)/ ARCH=arc O=$(O)/arc BOARD=arduino_101_sss
