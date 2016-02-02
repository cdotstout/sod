include project/target/dartuinoP0.mk
include project/virtual/fs.mk

MODULES += app/fletch app/shell

DARTINO_CONFIGURATION = LK
DARTINO_GYP_DEFINES = "LK_PATH=../../third_party/lk/ LK_PROJECT=dartuino-p0-fletch LK_CPU=cortex-m4"

WITH_CPP_SUPPORT=true
