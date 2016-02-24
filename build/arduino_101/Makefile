ZEPHYR_GCC_VARIANT ?= zephyr
ZEPHYR_SDK_INSTALL_DIR ?= /opt/zephyr-sdk

THIS_DIR := $(shell pwd)
O ?= $(THIS_DIR)/out

ZEPHYR_BASE ?= ../../

arc_app := $(ZEPHYR_BASE)/samples/nanokernel/apps/hello_world
x86_app := $(ZEPHYR_BASE)/samples/bluetooth/peripheral

image: x86_image arc_image

export ZEPHYR_SDK_INSTALL_DIR ZEPHYR_GCC_VARIANT ZEPHYR_BASE

clean:
	@rm -r -f out/

x86_image:
	bash -c "source $(ZEPHYR_BASE)/zephyr-env.sh; \
		mkdir -p $(O)/x86; \
		$(MAKE) -C $(x86_app)/ ARCH=x86 CONF_FILE=prj_nble.conf O=$(O)/x86 BOARD=arduino_101; \
	"

arc_image:
	bash -c "source $(ZEPHYR_BASE)/zephyr-env.sh; \
		mkdir -p $(O)/arc; \
		$(MAKE) -C $(arc_app)/ ARCH=arc O=$(O)/arc BOARD=arduino_101_sss; \
	"

flash:
	bash -c "source $(ZEPHYR_BASE)/zephyr-env.sh; \
		$(ZEPHYR_BASE)/boards/arduino_101/support/arduino_101_load.sh rom; \
		make -C $(x86_app) ARCH=x86 O=$(O)/x86 BOARD=arduino_101 flash; \
		make -C $(arc_app) ARCH=arc O=$(O)/arc BOARD=arduino_101_sss flash; \
	"

