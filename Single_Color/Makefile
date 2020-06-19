PROGRAM = single_color

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/dhcpserver \
	extras/pwm \
	$(abspath ../../components/esp-8266/wifi_config) \
	$(abspath ../../components/esp-8266/cJSON) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/homekit)

WHITE_PIN ?= 12

FLASH_SIZE ?= 32

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DWHITE_PIN=$(WHITE_PIN)

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)

