include project/target/dartuinoP0.mk
include project/virtual/fs.mk

MODULES += app/dartino app/shell

DARTINO_CONFIGURATION = LK
# Until lk enables FPU support, use soft (emulated) floating point.
DARTINO_GYP_DEFINES = "LK_PATH=../../third_party/lk/ LK_PROJECT=dartuino-p0-dartino LK_CPU=cortex-m4 LK_FPU=fpv4-sp-d16 LK_FLOAT-ABI=soft LK_USE_DEPS_ARM_GCC=0"

WITH_CPP_SUPPORT=true
