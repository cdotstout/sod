include project/target/stm32f746g-disco.mk
include project/virtual/fs.mk

MODULES += app/dartino app/shell

DARTINO_CONFIGURATION = LK
DARTINO_GYP_DEFINES = "LK_PATH=../../third_party/lk/ LK_PROJECT=stm32f746g-disco-dartino LK_CPU=cortex-m4 LK_USE_DEPS_ARM_GCC=0"

WITH_CPP_SUPPORT=true

# Console serial port is on pins PA9(TX) and PA10(RX)

