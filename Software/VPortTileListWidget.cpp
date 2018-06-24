#include "VPortTileListWidget.h"
#include <QScrollArea>

VPortTileListWidget::VPortTileListWidget( QWidget* parent /*= nullptr */ ) : QFrame( parent )
{
	QHBoxLayout* hLayout = new QHBoxLayout( this );
	hLayout->setContentsMargins( QMargins( 0, 0, 0, 0 ) );
	QScrollArea* scrollArea = new QScrollArea;
	scrollArea->setFrameShape( QFrame::NoFrame );
	scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	scrollArea->setWidgetResizable( true );
	hLayout->addWidget( scrollArea );

	QWidget* scrollAreaWidgetContents = new QWidget;
	QVBoxLayout* verticalLayout = new QVBoxLayout( scrollAreaWidgetContents );
	verticalLayout->setContentsMargins( 0, 0, 0, 0 );
	QHBoxLayout* horizontalLayout = new QHBoxLayout();
	QSpacerItem* horizontalSpacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout->addItem( horizontalSpacer );

	layout = new QVBoxLayout();
	layout->setContentsMargins( 9, 9, 9, 9 );
	horizontalLayout->addLayout( layout );

	QSpacerItem* horizontalSpacer_2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout->addItem( horizontalSpacer_2 );
	verticalLayout->addLayout( horizontalLayout );

	QSpacerItem* verticalSpacer = new QSpacerItem( 20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding );
	verticalLayout->addItem( verticalSpacer );

	scrollArea->setWidget( scrollAreaWidgetContents );
}

VPortTileListWidget::~VPortTileListWidget()
{

}

void VPortTileListWidget::setTilesCount( uint count )
{
	removeAllTiles();
	for( int i = 0; i < count; ++i )
		layout->addWidget( new VPortTileWidget( i ) );
}

void VPortTileListWidget::removeAllTiles()
{
	QLayoutItem* item;
	while( ( item = layout->takeAt( 0 ) ) )
	{
		delete item->widget();
		delete item;
	}
}

VPortTileWidget* VPortTileListWidget::tile( uint index )
{
	if( layout->count() <= index )
		return nullptr;
	return static_cast< VPortTileWidget* >( layout->itemAt( index )->widget() );
}