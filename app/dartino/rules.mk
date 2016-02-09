LOCAL_DIR := $(GET_LOCAL_DIR)

DARTINO_BASE := $(BUILDROOT)/../third_party/dartino

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

MODULE_INCLUDES += $(DARTINO_BASE)

ifneq ($(DEBUG),)
EXTRA_OBJS += $(DARTINO_BASE)/out/Debug$(DARTINO_CONFIGURATION)/libdartino.a
else
EXTRA_OBJS += $(DARTINO_BASE)/out/Release$(DARTINO_CONFIGURATION)/libdartino.a
endif

force_dartino_target: 

$(DARTINO_BASE)/out/Debug$(DARTINO_CONFIGURATION)/libdartino.a: force_dartino_target
	ninja -C $(DARTINO_BASE) lk -t clean
	GYP_DEFINES=$(DARTINO_GYP_DEFINES) ninja -C $(DARTINO_BASE) lk
	ninja -C $(DARTINO_BASE)/out/Debug$(DARTINO_CONFIGURATION)/ libdartino -t clean
	ninja -C $(DARTINO_BASE)/out/Debug$(DARTINO_CONFIGURATION)/ libdartino

$(DARTINO_BASE)/out/Release$(DARTINO_CONFIGURATION)/libdartino.a: force_dartino_target
	ninja -C $(DARTINO_BASE) lk -t clean
	GYP_DEFINES=$(DARTINO_GYP_DEFINES) ninja -C $(DARTINO_BASE) lk
	ninja -C $(DARTINO_BASE)/out/Release$(DARTINO_CONFIGURATION)/ libdartino -t clean
	ninja -C $(DARTINO_BASE)/out/Release$(DARTINO_CONFIGURATION)/ libdartino

# put arch local .S files here if developing memcpy/memmove

include make/module.mk

