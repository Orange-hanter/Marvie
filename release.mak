#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := Release

#Additional flags
PREPROCESSOR_MACROS := NDEBUG=1 RELEASE=1
INCLUDE_DIRS := os/common/ext/CMSIS/include os/common/ext/CMSIS/ST/STM32F4xx os/common/oslib/include os/common/ports/ARMCMx os/common/ports/ARMCMx/compilers/GCC os/common/startup/ARMCMx/devices/STM32F4xx os/license os/rt/include os/hal/include os/hal/osal/rt os/hal/ports/STM32/STM32F4xx os\hal\ports\STM32\LLD\ADCv2 os\hal\ports\STM32\LLD\CANv1 os\hal\ports\STM32\LLD\DACv1 os\hal\ports\STM32\LLD\DMAv2 os\hal\ports\STM32\LLD\EXTIv1 os\hal\ports\STM32\LLD\GPIOv2 os\hal\ports\STM32\LLD\I2Cv1 os\hal\ports\STM32\LLD\MACv1 os\hal\ports\STM32\LLD\OTGv1 os\hal\ports\STM32\LLD\QUADSPIv1 os\hal\ports\STM32\LLD\RTCv2 os\hal\ports\STM32\LLD\SDIOv1 os\hal\ports\STM32\LLD\SPIv1 os\hal\ports\STM32\LLD\TIMv1 os\hal\ports\STM32\LLD\USARTv1 os\hal\ports\STM32\LLD\xWDGv1 os\hal\ports\common\ARMCMx os/test/lib os/test/rt/source/test ext/fatfs/src os/various os/hal/lib/streams
LIBRARY_DIRS := os/common/startup/ARMCMx/compilers/GCC/ld
LIBRARY_NAMES := 
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS := -ggdb -ffunction-sections -O3
CXXFLAGS := -ggdb -ffunction-sections -fno-exceptions -fno-rtti -O3
ASFLAGS := 
LDFLAGS := -Wl,-gc-sections,--defsym=__main_stack_size__=0x800,--defsym=__process_stack_size__=0x800
COMMONFLAGS := 
LINKER_SCRIPT := os\common\startup\ARMCMx\compilers\GCC\ld\STM32F407xE.ld

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

#Additional options detected from testing the toolchain
USE_DEL_TO_CLEAN := 1
CP_NOT_AVAILABLE := 1

ADDITIONAL_MAKE_FILES := stm32.mak
GENERATE_BIN_FILE := 1
GENERATE_IHEX_FILE := 0
