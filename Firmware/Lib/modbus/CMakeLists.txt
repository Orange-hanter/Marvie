cmake_minimum_required(VERSION 3.11)

project(modbus)

set(MODBUS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MODBUS_INCLUDE_DIRS ${MODBUS_DIR})

add_library(modbus 
	ModbusASCII.cpp
	ModbusIP.cpp
	ModbusMaster.cpp
	ModbusRTU.cpp
	ModbusSlave.cpp
)
target_include_directories(modbus PUBLIC ${MODBUS_INCLUDE_DIRS})
