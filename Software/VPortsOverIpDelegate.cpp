#include "VPortsOverIpDelegate.h"
#include <QRegExpValidator>
#include <QLineEdit>
#include <QSpinBox>

VPortsOverIpDelegate::VPortsOverIpDelegate()
{

}

VPortsOverIpDelegate::~VPortsOverIpDelegate()
{

}

void VPortsOverIpDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
	QItemDelegate::paint( painter, option, index );
}

QWidget * VPortsOverIpDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
	if( index.column() == 0 )
	{
		QLineEdit* lineEdit = new QLineEdit( parent );
		lineEdit->setFrame( false );
		QRegExpValidator* validator = new QRegExpValidator( lineEdit );
		validator->setRegExp( QRegExp( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$" ) );
		lineEdit->setValidator( validator );
		return lineEdit;
	}
	else if( index.column() == 1 )
	{
		QSpinBox* spinBox = new QSpinBox( parent );
		spinBox->setFrame( false );
		spinBox->setMinimum( 1 );
		spinBox->setMaximum( 65535 );
		return spinBox;
	}

	return nullptr;
}

void VPortsOverIpDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
	if( index.column() == 0 )
	{
		QLineEdit* lineEdit = static_cast< QLineEdit* >( editor );
		lineEdit->setText( index.model()->data( index, Qt::EditRole ).toString() );
	}
	else if( index.column() == 1 )
	{
		QSpinBox* spinBox = static_cast< QSpinBox* >( editor );
		spinBox->setValue( index.model()->data( index, Qt::EditRole ).toInt() );
	}
}

void VPortsOverIpDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
	if( index.column() == 0 )
	{
		QLineEdit* lineEdit = static_cast< QLineEdit* >( editor );
		if( lineEdit->hasAcceptableInput() )
			model->setData( index, lineEdit->text(), Qt::EditRole );
	}
	else if( index.column() == 1 )
	{
		QSpinBox* spinBox = static_cast< QSpinBox* >( editor );
		model->setData( index, spinBox->value(), Qt::EditRole );
	}
}

void VPortsOverIpDelegate::updateEditorGeometry( QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QItemDelegate::updateEditorGeometry( editor, option, index );
}