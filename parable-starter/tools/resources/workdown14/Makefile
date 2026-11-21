TARGET = workdown14
CPP_SOURCES = $(wildcard *.cpp)
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
APP_TYPE = BOOT_NONE
CPP_STANDARD = -std=gnu++17
OPT = -O2
C_INCLUDES += -I$(LIBDAISY_DIR)/Middlewares/Patched/ST/STM32_USB_Device_Library/Class/CDC/Inc
include $(LIBDAISY_DIR)/core/Makefile
CPPFLAGS += -DPARABLE_RAT_BENCH_FIR97 -DPARABLE_RAT_BENCH_FLOATCLIP -fno-fast-math -fno-finite-math-only
