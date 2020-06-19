PROGRAM = magic_home_strip

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/multipwm \
	extras/dhcpserver \
	$(abspath ../../components/esp-8266/wifi_config) \
	$(abspath ../../components/esp-8266/cJSON) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/homekit)


RED_PIN ?= 5

GREEN_PIN ?= 13

BLUE_PIN ?= 14

WHITE_PIN ?= 12

FLASH_SIZE ?= 32

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DRED_PIN=$(RED_PIN) -DGREEN_PIN=$(GREEN_PIN) -DBLUE_PIN=$(BLUE_PIN) -DWHITE_PIN=$(WHITE_PIN)

include $(SDK_PATH)/common.mk

LIBS += m

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
