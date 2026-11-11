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

include $(LIBDAISY_DIR)/core/Makefile
# Preserve finite checks; do not enable fast-math in the real-time core.
CPPFLAGS += -fno-fast-math -fno-finite-math-only -fstack-usage
CFLAGS += -fstack-usage
ifeq ($(CODEC_LINK_PROBE),1)
# JSON errors throw only on the control side. Default minimal image stays no-exceptions.
CPPFLAGS += -fexceptions
endif
