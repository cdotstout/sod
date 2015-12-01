include project/target/dartuinoP0.mk

MODULES += app/fletch app/shell

FLETCH_CONFIGURATION = LK
FLETCH_GYP_DEFINES = "LK_PATH=../../third_party/lk/ LK_PROJECT=dartuino-p0-fletch LK_CPU=cortex-m4"

WITH_CPP_SUPPORT=true
