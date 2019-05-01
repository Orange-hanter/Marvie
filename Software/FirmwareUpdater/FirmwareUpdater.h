#pragma once

#include <QtWidgets/QWidget>
#include <QMap>
#include <QList>
#include "GroupUpdater.h"
#include "HostListModel.h"
#include "ui_FirmwareUpdater.h"

class FirmwareUpdater : public QWidget
{
	Q_OBJECT

public:
	FirmwareUpdater( QWidget* parent = Q_NULLPTR );

private slots:
	void openFolderButtonClicked();
	void addDevicesButtonClicked();
	void startButtonClicked();
	void treeViewMenuRequested( const QPoint& pos );
	void menuActionTriggered( QAction* action );

	void hostStateChanged( int index, GroupUpdater::HostState state );
	void hostVersions( int index, QString firmwareVersion, QString bootloaderVersion );
	void hostProgress( int index, float progress );
	void hostUpdated( int index );
	void allHostUpdated();

private:
	QString modelName( QString version );
	static QString toString( GroupUpdater::HostState state );

private:
	struct BinaryFileInfo
	{
		QString version;
		QString fullPath;
	};
	QMap< QString, QList< BinaryFileInfo > > bootloaderMap;
	QMap< QString, QList< BinaryFileInfo > > firmwareMap;
	HostListModel hostListModel;
	QVector< QPair< int, int > > hostGroupIndexMap;
	GroupUpdater groupUpdater;
	bool updateCompleted;

	QMenu* menu;
	Ui::FirmwareUpdaterClass ui;
};
