#include "MarviePlatform.h"

using namespace MarvieXmlConfigParsers;

const ComPortType MarviePlatform::comPortTypes[comPortsCount] = { ComPortType::Rs485, ComPortType::Rs485, ComPortType::Rs232 };
// const MarviePlatform::ComPortIoLines MarviePlatform::comPortIoLines[comPortsCount] = { { &SD3, IOPD9,  IOPD8/*IOPB10*/, IOPB15, IOPB14 },
//                                                                                        { &SD6, IOPC7, IOPC6,  IOPD13, IOPD12 },
//                                                                                        { &SD1, IOPA10, IOPA9,  IOPNA, IOPNA } };
 const MarviePlatform::ComPortIoLines MarviePlatform::comPortIoLines[comPortsCount] = { { &SD3, IOPD9,  IOPD8/*IOPB10*/, IOPB15, IOPB14 },
                                                                                        { &SD6, IOPC7, IOPC6,  IOPD13, IOPD12 },
                                                                                        { &SD1, IOPA10, IOPA9,  IOPNA, IOPNA } };

const char* MarviePlatform::configXmlRootTagName = "qxConfig";

void MarviePlatform::comPortAssignments( std::list< std::list< MarvieXmlConfigParsers::ComPortAssignment > >& comPortAssignments )
{
	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com0 = comPortAssignments.back();
	com0.push_back( ComPortAssignment::VPort );
	com0.push_back( ComPortAssignment::GsmModem );
	com0.push_back( ComPortAssignment::ModbusRtuSlave );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com1 = comPortAssignments.back();
	com1.push_back( ComPortAssignment::VPort );
	com1.push_back( ComPortAssignment::ModbusRtuSlave );

	comPortAssignments.push_back( std::list< ComPortAssignment >() );
	auto& com2 = comPortAssignments.back();
	com2.push_back( ComPortAssignment::VPort );
	com2.push_back( ComPortAssignment::ModbusRtuSlave );
	com2.push_back( ComPortAssignment::Multiplexer );
}