include project/target/stm32f746g-disco.mk

MODULES += app/fletch app/shell

DARTINO_CONFIGURATION = LK
DARTINO_GYP_DEFINES = "LK_PATH=../../third_party/lk/ LK_PROJECT=stm32f746g-disco-fletch LK_CPU=cortex-m4"

WITH_CPP_SUPPORT=true

# Console serial port is on pins PA9(TX) and PA10(RX)

