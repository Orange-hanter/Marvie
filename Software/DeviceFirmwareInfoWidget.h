#pragma once

#ifdef USE_FRAMELESS_WINDOW 
#include "FramelessWindow/FramelessWindow.h"
#endif
#include "ui_DeviceFirmwareInfoWidget.h"

#ifdef USE_FRAMELESS_WINDOW
class DeviceFirmwareInfoWidget : public FramelessWidget
#else
class DeviceFirmwareInfoWidget : public QWidget
#endif
{
public:
	DeviceFirmwareInfoWidget( QWidget* parent = nullptr );
	~DeviceFirmwareInfoWidget();

	void setFirmwareVersion( QString version );
	void setBootloaderVersion( QString version );
	void setModelName( QString name );
	void setSupportedSensorList( QStringList list );

	void clear();

private:
	enum { FirmwareRow, BootloaderRow, ModelRow, SensorsRow };
	Ui::Form ui;
};