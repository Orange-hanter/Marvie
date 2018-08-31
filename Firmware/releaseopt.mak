#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := ReleaseOpt

#Additional flags
PREPROCESSOR_MACROS := CHIBIOS=1 NDEBUG=1 RELEASE=1 USE_ASSERT=1
INCLUDE_DIRS := ChibiOS/common/ext/ARM/CMSIS/Core/Include ChibiOS/common/ext/ST/STM32F4xx ChibiOS/common/oslib/include ChibiOS/common/ports/ARMCMx ChibiOS/common/ports/ARMCMx/compilers/GCC ChibiOS/common/startup/ARMCMx/devices/STM32F4xx ChibiOS/license ChibiOS/rt/include ChibiOS/hal/include ChibiOS/hal/osal/rt ChibiOS/hal/ports/STM32/STM32F4xx ChibiOS/hal/ports/STM32/LLD/ADCv2 ChibiOS/hal/ports/STM32/LLD/CANv1 ChibiOS/hal/ports/STM32/LLD/CRCv1 ChibiOS/hal/ports/STM32/LLD/DACv1 ChibiOS/hal/ports/STM32/LLD/DMAv2 ChibiOS/hal/ports/STM32/LLD/EXTIv1 ChibiOS/hal/ports/STM32/LLD/GPIOv2 ChibiOS/hal/ports/STM32/LLD/I2Cv1 ChibiOS/hal/ports/STM32/LLD/MACv1 ChibiOS/hal/ports/STM32/LLD/OTGv1 ChibiOS/hal/ports/STM32/LLD/QUADSPIv1 ChibiOS/hal/ports/STM32/LLD/RTCv2 ChibiOS/hal/ports/STM32/LLD/SDIOv1 ChibiOS/hal/ports/STM32/LLD/SPIv1 ChibiOS/hal/ports/STM32/LLD/TIMv1 ChibiOS/hal/ports/STM32/LLD/USARTv1 ChibiOS/hal/ports/STM32/LLD/xWDGv1 ChibiOS/hal/ports/common/ARMCMx ChibiOS/test/lib ChibiOS/test/rt/source/test ChibiOS/test/oslib/source/test Lib/fatfs/src Lib/lwip/src/include ChibiOS/various ChibiOS/various/lwip_bindings ChibiOS/hal/lib/streams
LIBRARY_DIRS := ChibiOS/common/startup/ARMCMx/compilers/GCC/ld
LIBRARY_NAMES := 
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS := -ggdb -ffunction-sections -Og
CXXFLAGS := -ggdb -ffunction-sections -fno-exceptions -fno-rtti -Og -Wall
ASFLAGS := 
LDFLAGS := -Wl,-gc-sections,--defsym=__main_stack_size__=0x800,--defsym=__process_stack_size__=0x800
COMMONFLAGS := 
LINKER_SCRIPT := ChibiOS\common\startup\ARMCMx\compilers\GCC\ld\STM32F407xE.ld

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

#Additional options detected from testing the toolchain
USE_DEL_TO_CLEAN := 1
CP_NOT_AVAILABLE := 1

ADDITIONAL_MAKE_FILES := stm32.mak
GENERATE_BIN_FILE := 1
GENERATE_IHEX_FILE := 0
