#pragma once

#include <QWidget>
#include <QVector>
#include <QVariant>
#include <QBoxLayout>

class ComPortsConfigWidget : public QWidget
{
	Q_OBJECT
public:
	enum class Assignment { None, VPort, ModbusRtuSlave, GsmModem, Multiplexer };

	ComPortsConfigWidget( QWidget* parent = nullptr );
	~ComPortsConfigWidget();

	void init( QVector< QVector< Assignment > > portAssignments );
	int comPortsCount();
	bool setAssignment( unsigned int id, Assignment assignment );
	void setRelatedParameters( unsigned int id, const QVector< QVariant >& );
	Assignment assignment( unsigned int id );
	QVector< Assignment > assignments();
	QVector< QVariant > relatedParameters( unsigned int id );

signals:
	void assignmentChanged( unsigned int id, Assignment previous, Assignment current );
	void relatedParametersChanged( unsigned int id );

private:
	QString toString( Assignment a );
	Assignment toAssignment( QString s );
	void addContent( unsigned int id, QHBoxLayout* layout, Assignment assignment );
	void removeContent( QLayout* layout );

private slots:
	void assignmentComboBoxChanged( const QString& text );
	void relatedParameterChanged();

private:
	QVector< Assignment > assignmentsVect;
	QVBoxLayout* layout;
	static QStringList baudratesList;
	static QStringList formatsList;
};

