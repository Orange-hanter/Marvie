#pragma once

#include <QAbstractItemModel>
#include <QHostAddress>
#include <List>

class HostListModel : public QAbstractItemModel
{
public:
	struct Host 
	{
		QHostAddress addr;
		QString password;
		QString info;
		QString firmwareVersion;
		QString bootloaderVersion;
		int progress;
		QString state;
	};
	struct HostGroup
	{
		QString title;
		QList< Host > hostList;
	};

	HostListModel();
	~HostListModel();

	void setEditMode( bool enabled );

	void appendHostGroup( const HostGroup& hostList );
	const QList< HostGroup > hostGroupList() const;
	void setHostVersions( int groupIndex, int hostIndex, QString firmwareVersion, QString bootloaderVersion );
	void setHostProgress( int groupIndex, int hostIndex, int progress );
	void setHostState( int groupIndex, int hostIndex, QString state );
	void resetHostsStatus();
	void clear();

	QVariant data( const QModelIndex &index, int role ) const override;
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
	QModelIndex parent( const QModelIndex &index ) const override;

	int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
	int columnCount( const QModelIndex &parent = QModelIndex() ) const override;

	Qt::ItemFlags flags( const QModelIndex &index ) const override;
	bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;
	bool setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole ) override;

	bool insertColumns( int position, int columns, const QModelIndex &parent = QModelIndex() ) override;
	bool removeColumns( int position, int columns, const QModelIndex &parent = QModelIndex() ) override;
	bool insertRows( int position, int rows, const QModelIndex &parent = QModelIndex() ) override;
	bool removeRows( int position, int rows, const QModelIndex &parent = QModelIndex() ) override;

private:
	bool editMode;
	QList< HostGroup > list;
};