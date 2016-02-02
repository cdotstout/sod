# main project for qemu-arm
TARGET := qemu-virt
ARCH := arm
ARM_CPU := cortex-a15

MEMSIZE := 0x1000000  # 16MB

MODULES += \
	app/shell \
	app/fletch \
	lib/libm \
	lib/evlog

GLOBAL_DEFINES += WITH_KERNEL_EVLOG=1

DARTINO_CONFIGURATION = LKFull
DARTINO_GYP_DEFINES = "LK_PATH=../../third_party/lk/ LK_PROJECT=qemu-virt-fletch LK_CPU=cortex-a15"

WITH_CPP_SUPPORT=true

#WITH_LINKER_GC := 0

include project/virtual/fs.mk
