project(marvie)
cmake_minimum_required(VERSION 3.11)
enable_language(ASM)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

#set(STM32_FLASH_ORIGIN "0x08008400")
#set(STM32_FLASH_SIZE "479K")
#set(STM32_RAM_SIZE "128K")
#set(STM32_CCRAM_SIZE "64K")

set(WIN32 1)
#set(USE_PRINTF_FLOAT 1)
set(USE_SCANF_FLOAT 1)
include(gcc_stm32)

set(CHIBIOS_CONFIG_DIR "Config/Firmware")
find_package(ChibiOS 18 COMPONENTS rt hal REQUIRED)

include_directories(
	${ChibiOS_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various/lwip_bindings/
    ${CMAKE_SOURCE_DIR}/Config/
    ${CMAKE_SOURCE_DIR}/Config/Firmware
)

add_subdirectory(Lib/lwip)
add_subdirectory(Lib/modbus)
add_subdirectory(Lib/tinyxml2)
add_subdirectory(Lib/fatfs)
add_subdirectory(Lib/sha1)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/Src/
)

set(apps_SRCS
    ${CMAKE_SOURCE_DIR}/Src/Apps/Modbus/ModbusDevice.cpp
	${CMAKE_SOURCE_DIR}/Src/Apps/Modbus/ModbusServer.cpp
	${CMAKE_SOURCE_DIR}/Src/Apps/Modbus/RawModbusServer.cpp
	${CMAKE_SOURCE_DIR}/Src/Apps/Modbus/TcpModbusServer.cpp
)

set(core_SRCS
	${CMAKE_SOURCE_DIR}/Src/Core/BaseDynamicThread.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/ByteRingBuffer.cpp
	${CMAKE_SOURCE_DIR}/Src/Core/CCMemoryAllocator.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/CCMemoryHeap.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/CpuUsageMonitor.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/DateTime.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/DateTimeService.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/IODevice.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/NewDelete.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/Object.cpp
    ${CMAKE_SOURCE_DIR}/Src/Core/ObjectMemoryUtilizer.cpp
)

set(drivers_SRCS
	${CMAKE_SOURCE_DIR}/Src/Drivers/Interfaces/Rs485.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Interfaces/SharedRs485.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Interfaces/Usart.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Interfaces/UsartBasedDevice.cpp	
	${CMAKE_SOURCE_DIR}/Src/Drivers/Network/Ethernet/EthernetThread.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Network/SimGsm/SimGsmATResponseParsers.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Network/SimGsm/SimGsmPppModem.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/AbstractSensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Art01Sensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Ce301Sensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/GeneralSRSensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Mr01Sensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Tem05MSensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Tem104M1Sensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Tem104Sensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Tem104USensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/Term0204Sensor.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/TestSensorA.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/TestSensorB.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/Sensors/TestSRSensorA.cpp
	${CMAKE_SOURCE_DIR}/Src/Drivers/LogicOutput.cpp
)

set(fileSystem_SRCS
	${CMAKE_SOURCE_DIR}/Src/FileSystem/Dir.cpp
	${CMAKE_SOURCE_DIR}/Src/FileSystem/File.cpp
	${CMAKE_SOURCE_DIR}/Src/FileSystem/FileInfo.cpp
	${CMAKE_SOURCE_DIR}/Src/FileSystem/FileSystem.cpp
)

set(filters_SRCS
	${CMAKE_SOURCE_DIR}/Src/Filters/LowPassFilter.cpp
	${CMAKE_SOURCE_DIR}/Src/Filters/MedianFilter.cpp
	${CMAKE_SOURCE_DIR}/Src/Filters/MovingAvgFilter.cpp
)

set(log_SRCS
	${CMAKE_SOURCE_DIR}/Src/Log/FileLog.cpp
	${CMAKE_SOURCE_DIR}/Src/Log/MarvieLog.cpp
)

set(network_SRCS
	${CMAKE_SOURCE_DIR}/Src/Network/AbstractPppModem.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/AbstractSocket.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/AbstractTcpSocket.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/AbstractUdpSocket.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/IpAddress.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/LwipSocketPrivate.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/Modem.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/TcpServer.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/TcpSocket.cpp
	${CMAKE_SOURCE_DIR}/Src/Network/UdpSocket.cpp
)

set(sensorService_SRCS
	${CMAKE_SOURCE_DIR}/Src/SensorService/Art01SensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Ce301SensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/GeneralSRSensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Mr01SensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/SensorService.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Tem05MSensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Tem104M1SensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Tem104SensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Tem104USensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/Term0204SensorType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/TestSensorAType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/TestSensorBType.cpp
	${CMAKE_SOURCE_DIR}/Src/SensorService/TestSRSensorAType.cpp
)

set(support_SRCS
	${CMAKE_SOURCE_DIR}/Src/Support/LexicalAnalyzer.cpp
	${CMAKE_SOURCE_DIR}/Src/Support/Utility.cpp
)

set(chibios_various_SRCS
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various/lwip_bindings/arch/sys_arch.c
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various/cpp_wrappers/ch.cpp
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various/fatfs_bindings/fatfs_diskio.c
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various/fatfs_bindings/fatfs_syscall.c
	${CMAKE_SOURCE_DIR}/ChibiOS/os/various/evtimer.c
)

set(test_SRCS
	${CMAKE_SOURCE_DIR}/Tests/UdpStressTestServer/UdpStressTestServer.cpp
)

add_definitions(
	            -DCORTEX_USE_FPU=FALSE
                -DUSE_ASSERT=TRUE
)

set(STM32_LINKER_SCRIPT ${ChibiOS_LINKER_SCRIPT})

# Marvie firmware --------------------------------------------------------

set(firmware_SRCS
	${CMAKE_SOURCE_DIR}/Config/board.c
    ${CMAKE_SOURCE_DIR}/Src/BRSensorReader.cpp
    ${CMAKE_SOURCE_DIR}/Src/Crc32HW.cpp
    ${CMAKE_SOURCE_DIR}/Src/main.cpp
    ${CMAKE_SOURCE_DIR}/Src/MarvieDevice.cpp
	${CMAKE_SOURCE_DIR}/Src/MarviePlatform.cpp
	${CMAKE_SOURCE_DIR}/Src/MarvieBackup.cpp
    ${CMAKE_SOURCE_DIR}/Src/MarvieXmlConfigParsers.cpp
    ${CMAKE_SOURCE_DIR}/Src/MLinkServer.cpp
    ${CMAKE_SOURCE_DIR}/Src/MultipleBRSensorsReader.cpp
    ${CMAKE_SOURCE_DIR}/Src/NetworkSensorReader.cpp
    ${CMAKE_SOURCE_DIR}/Src/SingleBRSensorReader.cpp
	${CMAKE_SOURCE_DIR}/Src/stm32f4xx_flash.c
	#${CMAKE_SOURCE_DIR}/Src/usbcfg.c
)

add_executable(${CMAKE_PROJECT_NAME}_firmware.elf
	${firmware_SRCS}
	${apps_SRCS}
	${core_SRCS}
	${drivers_SRCS}
	${fileSystem_SRCS}
	${filters_SRCS}
	${log_SRCS}
	${network_SRCS}
	${sensorService_SRCS}
	${support_SRCS}
	${ChibiOS_SOURCES}
	${chibios_various_SRCS}
	${test_SRCS}
)
target_link_libraries(${CMAKE_PROJECT_NAME}_firmware.elf
    lwipcore
	fatfs
	modbus
	tinyxml2
	_sha1
)

STM32_SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME}_firmware.elf)
STM32_ADD_HEX_BIN_TARGETS(${CMAKE_PROJECT_NAME}_firmware.elf)
STM32_PRINT_SIZE_OF_TARGETS(${CMAKE_PROJECT_NAME}_firmware.elf)
