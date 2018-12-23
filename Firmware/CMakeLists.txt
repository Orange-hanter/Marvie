project(firmware)
cmake_minimum_required(VERSION 3.11)
enable_language(ASM)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(WIN32 1)
include(gcc_stm32)

find_package(ChibiOS 18 COMPONENTS rt hal REQUIRED)

include_directories(
	${ChibiOS_INCLUDE_DIRS}
	ChibiOS/os/various
	ChibiOS/os/various/lwip_bindings/
	Config/
)

add_subdirectory(Lib/lwip)
add_subdirectory(Lib/modbus)
add_subdirectory(Lib/tinyxml2)
add_subdirectory(Lib/fatfs)
add_subdirectory(Lib/sha1)

include_directories(
	Src/
	${CMAKE_SOURCE_DIR}
)

set(apps_SRCS
	Src/Apps/Modbus/ModbusDevice.cpp
	Src/Apps/Modbus/ModbusServer.cpp
	Src/Apps/Modbus/RawModbusServer.cpp
	Src/Apps/Modbus/TcpModbusServer.cpp
)

set(core_SRCS
    Src/Core/BaseDynamicThread.cpp
    Src/Core/ByteRingBuffer.cpp
	Src/Core/CCMemoryAllocator.cpp
    Src/Core/CCMemoryHeap.cpp
    Src/Core/CpuUsageMonitor.cpp
    Src/Core/DateTime.cpp
    Src/Core/DateTimeService.cpp
    Src/Core/IODevice.cpp
    Src/Core/NewDelete.cpp
    Src/Core/Object.cpp
    Src/Core/ObjectMemoryUtilizer.cpp
    Src/Core/RtcBackupRegisters.cpp
)

set(drivers_SRCS
	Src/Drivers/Interfaces/Rs485.cpp
	Src/Drivers/Interfaces/SharedRs485.cpp
	Src/Drivers/Interfaces/Usart.cpp
	Src/Drivers/Interfaces/UsartBasedDevice.cpp
	Src/Drivers/Network/Ethernet/EthernetThread.cpp
	Src/Drivers/Network/SimGsm/SimGsmATResponseParsers.cpp
	Src/Drivers/Network/SimGsm/SimGsmPppModem.cpp
	Src/Drivers/Sensors/AbstractSensor.cpp
	Src/Drivers/Sensors/CE301Sensor.cpp
	Src/Drivers/Sensors/Tem05MSensor.cpp
	Src/Drivers/Sensors/Tem104Sensor.cpp
	Src/Drivers/Sensors/TestSensorA.cpp
	Src/Drivers/Sensors/TestSensorB.cpp
	Src/Drivers/Sensors/TestSRSensorA.cpp
	Src/Drivers/LogicOutput.cpp
)

set(fileSystem_SRCS
	Src/FileSystem/Dir.cpp
	Src/FileSystem/File.cpp
	Src/FileSystem/FileInfo.cpp
	Src/FileSystem/FileSystem.cpp
)

set(log_SRCS
	Src/Log/FileLog.cpp
	Src/Log/MarvieLog.cpp
)

set(network_SRCS
	Src/Network/AbstractPppModem.cpp
	Src/Network/AbstractSocket.cpp
	Src/Network/AbstractTcpSocket.cpp
	Src/Network/AbstractUdpSocket.cpp
	Src/Network/IpAddress.cpp
	Src/Network/LwipSocketPrivate.cpp
	Src/Network/Modem.cpp
	Src/Network/TcpServer.cpp
	Src/Network/TcpSocket.cpp
	Src/Network/UdpSocket.cpp
)

set(sensorService_SRCS
	Src/SensorService/CE301Type.cpp
	Src/SensorService/SensorService.cpp
	Src/SensorService/Tem104Type.cpp
	Src/SensorService/TestSensorAType.cpp
	Src/SensorService/TestSensorBType.cpp
	Src/SensorService/TestSRSensorAType.cpp
)

set(support_SRCS
	Src/Support/LexicalAnalyzer.cpp
	Src/Support/Utility.cpp
)

set(chibios_various_SRCS
	ChibiOS/os/various/lwip_bindings/arch/sys_arch.c
	ChibiOS/os/various/cpp_wrappers/ch.cpp
	ChibiOS/os/various/fatfs_bindings/fatfs_diskio.c
	ChibiOS/os/various/fatfs_bindings/fatfs_syscall.c
	ChibiOS/os/various/evtimer.c
)

set(test_SRCS
	Tests/UdpStressTestServer/UdpStressTestServer.cpp
)

set(PROJECT_SOURCES
    Config/board.c
    Src/BRSensorReader.cpp
    Src/Crc32HW.cpp
    Src/main.cpp
    Src/MarvieDevice.cpp
    Src/MarviePlatform.cpp
    Src/MarvieXmlConfigParsers.cpp
    Src/MLinkServer.cpp
    Src/MultipleBRSensorsReader.cpp
    Src/NetworkSensorReader.cpp
    Src/SingleBRSensorReader.cpp
)

add_definitions(-DCORTEX_USE_FPU=FALSE -DUSE_ASSERT=TRUE)

set(STM32_LINKER_SCRIPT ${ChibiOS_LINKER_SCRIPT})

add_executable(${CMAKE_PROJECT_NAME}.elf 
	${PROJECT_SOURCES}
	${apps_SRCS} 
	${core_SRCS} 
	${drivers_SRCS} 
	${fileSystem_SRCS} 
	${log_SRCS} 
	${network_SRCS} 
	${sensorService_SRCS} 
	${support_SRCS} 
	${ChibiOS_SOURCES}
	${chibios_various_SRCS}
	${test_SRCS}
)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf 
                      lwipcore 
					  fatfs 
					  modbus 
					  tinyxml2 
					  _sha1
)

STM32_SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME}.elf)
STM32_ADD_HEX_BIN_TARGETS(${CMAKE_PROJECT_NAME}.elf)
STM32_PRINT_SIZE_OF_TARGETS(${CMAKE_PROJECT_NAME}.elf)
