#include "ComPortsConfigWidget.h"
#include <QIntValidator>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>

QStringList ComPortsConfigWidget::baudratesList = []()
{
	QStringList list;
	list.append( "110" );
	list.append( "150" );
	list.append( "300" );
	list.append( "1200" );
	list.append( "2400" );
	list.append( "4800" );
	list.append( "9600" );
	list.append( "19200" );
	list.append( "38400" );
	list.append( "57600" );
	list.append( "115200" );

	return list;
}();

QStringList ComPortsConfigWidget::formatsList = []()
{
	QStringList list;
	list.append( "B7E" );
	list.append( "B7O" );
	list.append( "B8N" );
	list.append( "B8E" );
	list.append( "B8O" );

	return list;
}();


ComPortsConfigWidget::ComPortsConfigWidget( QWidget* parent ) : QWidget( parent )
{
	layout = new QVBoxLayout( this );
	layout->setContentsMargins( QMargins( 0, 0, 0, 0 ) );
}

ComPortsConfigWidget::~ComPortsConfigWidget()
{

}

void ComPortsConfigWidget::init( QVector< QVector< Assignment > > portAssignments )
{
	removeContent( layout );
	for( int i = 0; i < portAssignments.size(); ++i )
	{
		QHBoxLayout* hLayout = new QHBoxLayout;
		QLabel* label = new QLabel( QString( "COM%1" ).arg( i ) );
		label->setFixedWidth( 35 );
		hLayout->addWidget( label );
		QComboBox* comboBox = new QComboBox;
		comboBox->setFixedHeight( 23 );
		comboBox->setObjectName( QString( "%1" ).arg( i ) );
		for( const auto& i2 : portAssignments[i] )
			comboBox->addItem( toString( i2 ) );
		hLayout->addWidget( comboBox );
		hLayout->addItem( new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
		QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::assignmentComboBoxChanged );

		QHBoxLayout* hContentLayout = new QHBoxLayout;
		hContentLayout->setObjectName( QString( "%1.content" ).arg( i ) );
		hContentLayout->setContentsMargins( QMargins( 35, 0, 0, 0 ) );
		addContent( i, hContentLayout, portAssignments[i][0] );

		layout->addLayout( hLayout );
		layout->addLayout( hContentLayout );

		if( i < assignmentsVect.size() )
		{
			auto prev = assignmentsVect[i];
			assignmentsVect[i] = portAssignments[i][0];
			assignmentChanged( i, prev, assignmentsVect[i] );
			relatedParametersChanged( i );
		}
		else
		{
			assignmentsVect.append( portAssignments[i][0] );
			assignmentChanged( i, Assignment::None, assignmentsVect[i] );
			relatedParametersChanged( i );
		}
	}

	for( int i = portAssignments.size(); i < assignmentsVect.size(); ++i )
		assignmentChanged( i, assignmentsVect[i], Assignment::None );
	assignmentsVect = assignmentsVect.mid( 0, portAssignments.size() );
}

int ComPortsConfigWidget::comPortsCount()
{
	return assignmentsVect.size();
}

bool ComPortsConfigWidget::setAssignment( unsigned int id, Assignment assignment )
{
	QComboBox* comboBox = findChild< QComboBox* >( QString( "%1" ).arg( id ) );
	int i = comboBox->findText( toString( assignment ) );
	if( i != -1 )
	{
		comboBox->setCurrentIndex( i );
		return true;
	}

	return false;
}

void ComPortsConfigWidget::setRelatedParameters( unsigned int id, const QMap< QString, QVariant >& values )
{
	switch( assignmentsVect[id] )
	{
	case ComPortsConfigWidget::Assignment::VPort:
		if( values.contains( "format" ) )
			findChild< QComboBox* >( QString( "%1.format" ).arg( id ) )->setCurrentText( values["format"].toString() );
		if( values.contains( "baudrate" ) )
			findChild< QComboBox* >( QString( "%1.baudrate" ).arg( id ) )->setCurrentText( QString( "%1" ).arg( values["baudrate"].toInt() ) );
		break;
	case ComPortsConfigWidget::Assignment::ModbusRtuSlave:
	case ComPortsConfigWidget::Assignment::ModbusAsciiSlave:
		if( values.contains( "format" ) )
			findChild< QComboBox* >( QString( "%1.format" ).arg( id ) )->setCurrentText( values["format"].toString() );
		if( values.contains( "baudrate" ) )
			findChild< QComboBox* >( QString( "%1.baudrate" ).arg( id ) )->setCurrentText( QString( "%1" ).arg( values["baudrate"].toInt() ) );
		if( values.contains( "address" ) )
			findChild< QSpinBox* >( QString( "%1.address" ).arg( id ) )->setValue( values["address"].toInt() );
		break;
	case ComPortsConfigWidget::Assignment::GsmModem:
		if( values.contains( "pinCode" ) )
			findChild< QLineEdit* >( QString( "%1.pinCode" ).arg( id ) )->setText( values["pinCode"].toString() );
		if( values.contains( "apn" ) )
			findChild< QLineEdit* >( QString( "%1.apn" ).arg( id ) )->setText( values["apn"].toString() );
		break;
	case ComPortsConfigWidget::Assignment::Multiplexer:
		for( int i = 0; i < 5; ++i )
		{
			QString formatPrmName = QString( "%1.format" ).arg( i );
			QString baudratePrmName = QString( "%1.baudrate" ).arg( i );
			if( values.contains( formatPrmName ) )
				findChild< QComboBox* >( QString( "%1.%2.format" ).arg( id ).arg( i ) )->setCurrentText( values[formatPrmName].toString() );
			if( values.contains( baudratePrmName ) )
				findChild< QComboBox* >( QString( "%1.%2.baudrate" ).arg( id ).arg( i ) )->setCurrentText( QString( "%1" ).arg( values[baudratePrmName].toInt() ) );
		}
		break;
	default:
		break;
	}
	relatedParametersChanged( id );
}

ComPortsConfigWidget::Assignment ComPortsConfigWidget::assignment( unsigned int id )
{
	return assignmentsVect[id];
}

QVector< ComPortsConfigWidget::Assignment > ComPortsConfigWidget::assignments()
{
	return assignmentsVect;
}

QMap< QString, QVariant > ComPortsConfigWidget::relatedParameters( unsigned int id )
{
	QMap< QString, QVariant > prms;
	switch( assignmentsVect[id] )
	{
	case ComPortsConfigWidget::Assignment::VPort:
		prms.insert( "format", findChild< QComboBox* >( QString( "%1.format" ).arg( id ) )->currentText() );
		prms.insert( "baudrate", findChild< QComboBox* >( QString( "%1.baudrate" ).arg( id ) )->currentText().toInt() );
		break;
	case ComPortsConfigWidget::Assignment::ModbusRtuSlave:
	case ComPortsConfigWidget::Assignment::ModbusAsciiSlave:
		prms.insert( "format", findChild< QComboBox* >( QString( "%1.format" ).arg( id ) )->currentText() );
		prms.insert( "baudrate", findChild< QComboBox* >( QString( "%1.baudrate" ).arg( id ) )->currentText().toInt() );
		prms.insert( "address", findChild< QSpinBox* >( QString( "%1.address" ).arg( id ) )->value() );
		break;
	case ComPortsConfigWidget::Assignment::GsmModem:
		prms.insert( "pinCode", findChild< QLineEdit* >( QString( "%1.pinCode" ).arg( id ) )->text() );
		prms.insert( "apn", findChild< QLineEdit* >( QString( "%1.apn" ).arg( id ) )->text() );
		break;
	case ComPortsConfigWidget::Assignment::Multiplexer:
		for( int i = 0; i < 5; ++i )
		{
			prms.insert( QString( "%1.format" ).arg( i ), findChild< QComboBox* >( QString( "%1.%2.format" ).arg( id ).arg( i ) )->currentText() );
			prms.insert( QString( "%1.baudrate" ).arg( i ), findChild< QComboBox* >( QString( "%1.%2.baudrate" ).arg( id ).arg( i ) )->currentText().toInt() );
		}
		break;
	default:
		break;
	}

	return prms;
}

QString ComPortsConfigWidget::toString( Assignment a )
{
	switch( a )
	{
	case ComPortsConfigWidget::Assignment::VPort:
		return "VPort";
	case ComPortsConfigWidget::Assignment::ModbusRtuSlave:
		return "ModbusRtuSlave";
	case ComPortsConfigWidget::Assignment::ModbusAsciiSlave:
		return "ModbusAsciiSlave";
	case ComPortsConfigWidget::Assignment::GsmModem:
		return "GsmModem";
	case ComPortsConfigWidget::Assignment::Multiplexer:
		return "Multiplexer";
	default:
		return "";
	}
}

ComPortsConfigWidget::Assignment ComPortsConfigWidget::toAssignment( QString s )
{
	if( s == "VPort" )
		return Assignment::VPort;
	else if( s == "ModbusRtuSlave" )
		return Assignment::ModbusRtuSlave;
	else if( s == "ModbusAsciiSlave" )
		return Assignment::ModbusAsciiSlave;
	else if( s == "GsmModem" )
		return Assignment::GsmModem;
	else if( s == "Multiplexer" )
		return Assignment::Multiplexer;
	return Assignment::None;
}

void ComPortsConfigWidget::addContent( unsigned int id, QHBoxLayout* layout, Assignment assignment )
{
	removeContent( layout );
	switch( assignment )
	{
	case ComPortsConfigWidget::Assignment::VPort:
	{
		layout->addWidget( new QLabel( "Format" ) );
		QComboBox* comboBox = new QComboBox;
		comboBox->setObjectName( QString( "%1.format" ).arg( id ) );
		comboBox->setFixedHeight( 23 );
		comboBox->addItems( formatsList );
		comboBox->setCurrentText( "B8N" );
		QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( comboBox );

		layout->addWidget( new QLabel( "Baudrate" ) );
		comboBox = new QComboBox;
		comboBox->setObjectName( QString( "%1.baudrate" ).arg( id ) );
		comboBox->setFixedHeight( 23 );
		comboBox->addItems( baudratesList );
		comboBox->setCurrentText( "9600" );
		QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( comboBox );

		layout->addItem( new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
		break;
	}
	case ComPortsConfigWidget::Assignment::ModbusRtuSlave:
	case ComPortsConfigWidget::Assignment::ModbusAsciiSlave:
	{
		layout->addWidget( new QLabel( "Format" ) );
		QComboBox* comboBox = new QComboBox;
		comboBox->setObjectName( QString( "%1.format" ).arg( id ) );
		comboBox->setFixedHeight( 23 );
		comboBox->addItems( formatsList );
		comboBox->setCurrentText( "B8N" );
		QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( comboBox );

		layout->addWidget( new QLabel( "Baudrate" ) );
		comboBox = new QComboBox;
		comboBox->setObjectName( QString( "%1.baudrate" ).arg( id ) );
		comboBox->setFixedHeight( 23 );
		comboBox->addItems( baudratesList );
		comboBox->setCurrentText( "9600" );
		QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( comboBox );

		layout->addWidget( new QLabel( "Address" ) );
		QSpinBox* spinBox = new QSpinBox;
		spinBox->setObjectName( QString( "%1.address" ).arg( id ) );
		spinBox->setFixedHeight( 23 );
		spinBox->setMinimum( 0 );
		spinBox->setMaximum( 255 );
		QObject::connect( spinBox, static_cast< void( QSpinBox::* )( int ) >( &QSpinBox::valueChanged ), this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( spinBox );

		layout->addItem( new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
		break;
	}
	case ComPortsConfigWidget::Assignment::GsmModem:
	{
		layout->addWidget( new QLabel( "Pin code" ) );
		QLineEdit* edit = new QLineEdit;
		edit->setObjectName( QString( "%1.pinCode" ).arg( id ) );
		edit->setFixedHeight( 23 );
		edit->setValidator( new QIntValidator( 0, 9999, edit ) );
		QObject::connect( edit, &QLineEdit::textChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( edit );

		layout->addWidget( new QLabel( "APN" ) );
		edit = new QLineEdit;
		edit->setObjectName( QString( "%1.apn" ).arg( id ) );
		edit->setFixedHeight( 23 );
		QObject::connect( edit, &QLineEdit::textChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
		layout->addWidget( edit );

		layout->addItem( new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
		layout->setStretch( 4, 1 );
		break;
	}
	case ComPortsConfigWidget::Assignment::Multiplexer:
	{
		QVBoxLayout* vLayout = new QVBoxLayout;
		vLayout->setContentsMargins( QMargins( 0, 0, 0, 0 ) );
		layout->addLayout( vLayout );

		for( int i = 0; i < 5; ++i )
		{
			QHBoxLayout* hLayout = new QHBoxLayout;

			hLayout->addWidget( new QLabel( QString( "COM%1:  Format" ).arg( i ) ) );
			QComboBox* comboBox = new QComboBox;
			comboBox->setObjectName( QString( "%1.%2.format" ).arg( id ).arg( i ) );
			comboBox->setFixedHeight( 23 );
			comboBox->addItems( formatsList );
			comboBox->setCurrentText( "B8N" );
			QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
			hLayout->addWidget( comboBox );

			hLayout->addWidget( new QLabel( "Baudrate" ) );
			comboBox = new QComboBox;
			comboBox->setObjectName( QString( "%1.%2.baudrate" ).arg( id ).arg( i ) );
			comboBox->setFixedHeight( 23 );
			comboBox->addItems( baudratesList );
			comboBox->setCurrentText( "9600" );
			QObject::connect( comboBox, &QComboBox::currentTextChanged, this, &ComPortsConfigWidget::relatedParameterChanged );
			hLayout->addWidget( comboBox );

			hLayout->addItem( new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
			vLayout->addLayout( hLayout );
		}
		break;
	}
	default:
		break;
	}
}

void ComPortsConfigWidget::removeContent( QLayout* layout )
{
	QLayoutItem* item;
	while( ( item = layout->takeAt( 0 ) ) )
	{
		if( item->layout() )
			removeContent( item->layout() ), delete item->layout();
		else
			delete item->widget(), delete item;
	}
}

void ComPortsConfigWidget::assignmentComboBoxChanged( const QString& text )
{
	unsigned int id = sender()->objectName().toInt();
	auto prms = relatedParameters( id );
	addContent( id, findChild< QHBoxLayout* >( QString( "%1.content" ).arg( id ) ), toAssignment( text ) );
	auto prev = assignmentsVect[id];
	assignmentsVect[id] = toAssignment( text );
	setRelatedParameters( id, prms );
	assignmentChanged( id, prev, assignmentsVect[id] );
}

void ComPortsConfigWidget::relatedParameterChanged()
{
	unsigned int id = sender()->objectName().split( '.' )[0].toInt();
	relatedParametersChanged( id );
}