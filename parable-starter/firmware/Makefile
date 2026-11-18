TARGET = parable_seed3_minimal
ifndef LIBDAISY_DIR
$(error Run python3 firmware/build.py or provide the pinned LIBDAISY_DIR explicitly)
endif
CPP_SOURCES = daisy_pass_through.cpp ../src/parable_rat.cpp
C_INCLUDES = -I../include
C_DEFS = -DNDEBUG
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
CPP_STANDARD = -std=gnu++17
APP_TYPE ?= BOOT_NONE
OPT = -O2

ifeq ($(CODEC_LINK_PROBE),1)
TARGET = parable_seed3_codec_probe
CPP_SOURCES += ../src/parable_patch.cpp ../src/patch_json.cpp ../src/parable_prepare.cpp
C_INCLUDES += -I$(JSON_DIR)/include
C_DEFS += -DPARABLE_CODEC_LINK_PROBE
endif

ifeq ($(RAT_LINK_PROBE),1)
TARGET = parable_seed3_rat_probe
C_DEFS += -DPARABLE_RAT_LINK_PROBE
endif

ifneq ($(FIRST_BOOT_IMAGE),)
ifneq ($(filter 1,$(CODEC_LINK_PROBE) $(RAT_LINK_PROBE)),)
$(error FIRST_BOOT_IMAGE cannot be combined with legacy link-probe selectors)
endif
ifneq ($(APP_TYPE),BOOT_NONE)
$(error FIRST_BOOT_IMAGE requires the audited BOOT_NONE internal-flash layout)
endif
C_INCLUDES += -I$(LIBDAISY_DIR)/Middlewares/Patched/ST/STM32_USB_Device_Library/Class/CDC/Inc
ifeq ($(FIRST_BOOT_IMAGE),led_recovery)
TARGET = parable_seed3_led
CPP_SOURCES = seed3_led.cpp
else ifeq ($(FIRST_BOOT_IMAGE),mono_bypass)
TARGET = parable_seed3_bypass
C_DEFS += -DPARABLE_FIRST_BOOT
else ifeq ($(FIRST_BOOT_IMAGE),rat_composite)
TARGET = parable_seed3_rat_composite
C_DEFS += -DPARABLE_FIRST_BOOT -DPARABLE_RAT_LINK_PROBE
else ifeq ($(FIRST_BOOT_IMAGE),rat_composite_1x)
TARGET = parable_seed3_rat_composite_1x
C_DEFS += -DPARABLE_FIRST_BOOT -DPARABLE_RAT_LINK_PROBE -DPARABLE_RAT_BENCH_1X
else
$(error Unknown FIRST_BOOT_IMAGE; use led_recovery, mono_bypass, rat_composite or rat_composite_1x)
endif
endif

include $(LIBDAISY_DIR)/core/Makefile
# Preserve finite checks; do not enable fast-math in the real-time core.
CPPFLAGS += -fno-fast-math -fno-finite-math-only -fstack-usage
CFLAGS += -fstack-usage
ifeq ($(CODEC_LINK_PROBE),1)
# JSON errors throw only on the control side. Default minimal image stays no-exceptions.
CPPFLAGS += -fexceptions
endif
