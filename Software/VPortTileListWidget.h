#pragma once

#include "VPortTileWidget.h"

class VPortTileListWidget : public QFrame
{
public:
	VPortTileListWidget( QWidget* parent = nullptr );
	~VPortTileListWidget();

	void setTilesCount( uint count );
	void removeAllTiles();

	VPortTileWidget* tile( uint index );

private:
	QVBoxLayout * layout;
};
