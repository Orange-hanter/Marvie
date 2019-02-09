project(bootloader)
cmake_minimum_required(VERSION 3.11)
enable_language(ASM)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(STM32_FLASH_ORIGIN "0x08000000")
set(STM32_FLASH_SIZE "32K")
set(STM32_RAM_SIZE "128K")
set(STM32_CCRAM_SIZE "64K")

#set(WIN32 1)
include(gcc_stm32)

set(CHIBIOS_CONFIG_DIR "../Config/Bootloader")
find_package(ChibiOS 18 COMPONENTS rt hal REQUIRED)

set(marvie_dir ${CMAKE_SOURCE_DIR}/..)

include_directories(
	${ChibiOS_INCLUDE_DIRS}
	${marvie_dir}/ChibiOS/os/various
	${marvie_dir}/ChibiOS/os/various/lwip_bindings/
	${marvie_dir}/Config/
	${marvie_dir}/Config/Bootloader
)

add_subdirectory(../Lib/fatfs  ${CMAKE_BINARY_DIR}/Lib/fatfs)
add_subdirectory(../Lib/sha1  ${CMAKE_BINARY_DIR}/Lib/sha1)

include_directories(
	Src/
)

add_definitions(-DCORTEX_USE_FPU=FALSE -DUSE_ASSERT=TRUE)

set(STM32_LINKER_SCRIPT ${ChibiOS_LINKER_SCRIPT})

# Marvie bootloader -----------------------------------------------------

set(bootloader_SRCS
	${marvie_dir}/Config/board.c
	${CMAKE_SOURCE_DIR}/Src/bootloader.cpp
	${CMAKE_SOURCE_DIR}/Src/stm32f4xx_flash.c

	${marvie_dir}/ChibiOS/os/various/cpp_wrappers/ch.cpp
	${marvie_dir}/ChibiOS/os/various/fatfs_bindings/fatfs_diskio.c
	${marvie_dir}/ChibiOS/os/various/fatfs_bindings/fatfs_syscall.c
	${marvie_dir}/ChibiOS/os/various/evtimer.c

	${marvie_dir}/Src/Core/NewDelete.cpp
)

add_executable(${CMAKE_PROJECT_NAME}.elf
	${bootloader_SRCS}
	${core_SRCS}	
	${fileSystem_SRCS}
	
	${support_SRCS}
	${ChibiOS_SOURCES}
)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf
					  fatfs
)

STM32_SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME}.elf)
STM32_ADD_HEX_BIN_TARGETS(${CMAKE_PROJECT_NAME}.elf)
STM32_PRINT_SIZE_OF_TARGETS(${CMAKE_PROJECT_NAME}.elf)