#ifndef AIRVELOCITYWIDGET_H
#define AIRVELOCITYWIDGET_H

#include <QWidget>
#include "ui_AirVelocityWidget.h"

class senseSoarMAV;
class UASInterface;

class AirVelocityWidget : public QWidget
{
	Q_OBJECT

public:
	AirVelocityWidget(QWidget *parent = 0);
	~AirVelocityWidget();

private:
	Ui::AirVelocityWidget ui;
	senseSoarMAV* m_mav;


private slots:
	void UpdateAirSpeed(float Speed);
	void UpdateAoA(float AoA);
	void UpdateSideSlip(float Beta);
	void setUAS(UASInterface* uas);
};

#endif // AIRVELOCITYWIDGET_H
