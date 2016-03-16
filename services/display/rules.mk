LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
    external/gbskia \

MODULE_DEFINES +=  \
    SK_BUILD_FOR_LK \

MODULE_CPPFLAGS += -std=c++11

MODULE_SRCS += \
    $(LOCAL_DIR)/displayable.cpp \
    $(LOCAL_DIR)/displaym.cpp \

include make/module.mk
