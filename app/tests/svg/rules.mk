LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
    lib/libm    \
    external/gbskia \
    external/gbskia/tests \

MODULE_DEFINES +=  \
    SK_BUILD_FOR_LK \

MODULE_CPPFLAGS += -std=c++11

MODULE_SRCS += \
    $(LOCAL_DIR)/svg_test.cpp \

include make/module.mk
