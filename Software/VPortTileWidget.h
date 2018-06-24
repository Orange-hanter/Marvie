#pragma once

#include <QProgressBar>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTableView>
#include <QWidget>
#include <QLabel>
#include <SensorErrorsModel.h>

class VPortTileWidget : public QWidget
{
	Q_OBJECT

public:
	enum class State {};
	typedef SensorErrorsModel::SensorError SensorError;

	VPortTileWidget( QWidget* parent = nullptr );
	VPortTileWidget( uint index, QWidget* parent = nullptr );
	~VPortTileWidget();

	void setVPortIndex( uint index );
	void setBindInfo( QString bindInfo );
	void setState( State state );

	void setNextSensorRead( uint sensorId, QString sensorName, uint timeLeftSec );
	void resetNextSensorRead();

	void addSensorReadError( uint sensorId, QString sensorName, SensorError error, QDateTime date );
	void removeSensorReadError( uint sensorId );
	void clearSensorErrorsList();

private slots:
	void buttonClicked();

private:
	bool eventFilter( QObject* obj, QEvent* e ) override;
	QString toText( State state );

private:
	QLabel* vPortLabel;
	QLabel* statusLabel;
	QLabel* sensorNameLabel;
	QProgressBar *progressBar;
	QLabel* timeLeftLabel;
	QFrame* popupWindow;
	QLabel* bindLabel;
	QLabel* stateLabel;
	QTableView* sensorErrorsListTableView;
	SensorErrorsModel model;
	uint timeLeft;
	int nextSensorId;
};