#include "MarviePlatform.h"

using namespace MarvieXmlConfigParsers;

const IOPort MarviePlatform::analogInputAddressSelectorPorts[4] = { IOPE3, IOPE4, IOPE5, IOPE6 };

const IOPort MarviePlatform::digitInputPorts[digitInputsCount] = { IOPE8, IOPE9, IOPE10, IOPE11, IOPE12, IOPE13, IOPE14, IOPE15 };

const MarviePlatform::ComUsartIoLines MarviePlatform::comUsartIoLines[comUsartsCount] = { { &SD2, IOPD6, IOPD5 },
                                                                                          { &SD6, IOPC7, IOPC6 },
                                                                                          { &SD3, IOPD9, IOPD8 },
                                                                                          { &SD1, IOPA10, IOPA9 } };

const ComPortType MarviePlatform::comPortTypes[comPortsCount] = { ComPortType::Rs232,
                                                                  ComPortType::Rs485, 
                                                                  ComPortType::Rs485,
                                                                  ComPortType::Rs485,
                                                                  ComPortType::Rs485, 
                                                                  ComPortType::Rs485, 
																  ComPortType::Rs232 };

const MarviePlatform::ComPortIoLines MarviePlatform::comPortIoLines[comPortsCount] = { { 0, IOPNA, IOPNA },
																					   { 1, IOPD11, IOPD10 },
																					   { 1, IOPD13, IOPD12 },
																					   { 1, IOPD15, IOPD14 },
																					   { 1, IOPD4, IOPD3 },
																					   { 2, IOPB15, IOPB14 },
																					   { 3, IOPNA, IOPNA } };

const IOPort MarviePlatform::gsmModemEnableIoPort = IOPD0;

void MarviePlatform::comPortAssignments( std::list< std::list< MarvieXmlConfigParsers::ComPortAssignment > >& comPortAssignments )
{
	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com0 = comPortAssignments.back();
	com0.push_back( ComPortAssignment::GsmModem );
	com0.push_back( ComPortAssignment::ModbusRtuSlave );
	com0.push_back( ComPortAssignment::ModbusAsciiSlave );
	com0.push_back( ComPortAssignment::VPort );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com1 = comPortAssignments.back();
	com1.push_back( ComPortAssignment::VPort );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com2 = comPortAssignments.back();
	com2.push_back( ComPortAssignment::VPort );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com3 = comPortAssignments.back();
	com3.push_back( ComPortAssignment::VPort );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com4 = comPortAssignments.back();
	com4.push_back( ComPortAssignment::VPort );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com5 = comPortAssignments.back();
	com5.push_back( ComPortAssignment::VPort );
	com5.push_back( ComPortAssignment::ModbusRtuSlave );
	com5.push_back( ComPortAssignment::ModbusAsciiSlave );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com6 = comPortAssignments.back();
	com6.push_back( ComPortAssignment::VPort );
	com6.push_back( ComPortAssignment::ModbusRtuSlave );
	com6.push_back( ComPortAssignment::ModbusAsciiSlave );
}