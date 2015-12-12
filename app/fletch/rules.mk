LOCAL_DIR := $(GET_LOCAL_DIR)

FLETCH_BASE := $(BUILDROOT)/../third_party/fletch

MODULE := $(LOCAL_DIR)

MODULE_CPPFLAGS := -std=c++11

MODULE_DEPS += \
    lib/libm \
    lib/tftp \
    lib/font \

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c \
	$(LOCAL_DIR)/missing.c \
	$(LOCAL_DIR)/loader.cpp \
	$(LOCAL_DIR)/sensors.c

MODULE_INCLUDES += $(FLETCH_BASE)

ifneq ($(DEBUG),)
EXTRA_OBJS += $(FLETCH_BASE)/out/Debug$(FLETCH_CONFIGURATION)/libfletch.a
else
EXTRA_OBJS += $(FLETCH_BASE)/out/Release$(FLETCH_CONFIGURATION)/libfletch.a
endif

force_fletch_target: 

$(FLETCH_BASE)/out/Debug$(FLETCH_CONFIGURATION)/libfletch.a: force_fletch_target
	ninja -C $(FLETCH_BASE) lk -t clean
	GYP_DEFINES=$(FLETCH_GYP_DEFINES) ninja -C $(FLETCH_BASE) lk
	ninja -C $(FLETCH_BASE)/out/Debug$(FLETCH_CONFIGURATION)/ libfletch -t clean
	ninja -C $(FLETCH_BASE)/out/Debug$(FLETCH_CONFIGURATION)/ libfletch

$(FLETCH_BASE)/out/Release$(FLETCH_CONFIGURATION)/libfletch.a: force_fletch_target
	ninja -C $(FLETCH_BASE) lk -t clean
	GYP_DEFINES=$(FLETCH_GYP_DEFINES) ninja -C $(FLETCH_BASE) lk
	ninja -C $(FLETCH_BASE)/out/Release$(FLETCH_CONFIGURATION)/ libfletch -t clean
	ninja -C $(FLETCH_BASE)/out/Release$(FLETCH_CONFIGURATION)/ libfletch

# put arch local .S files here if developing memcpy/memmove

include make/module.mk

