#include "AirVelocityWidget.h"
#include "UASManager.h"
#include "senseSoarMAV.h"

AirVelocityWidget::AirVelocityWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	// connect with mavlink message
	connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));

	this->setVisible(false);
}

AirVelocityWidget::~AirVelocityWidget()
{

}

void AirVelocityWidget::UpdateAirSpeed(float Speed)
{
	ui.AirSpeedLabel->setText(tr("%1").arg(Speed));
}

void AirVelocityWidget::UpdateAoA(float aoa)
{
	ui.AoALabel->setText(tr("%1").arg(aoa));
}

void AirVelocityWidget::UpdateSideSlip(float beta)
{
	ui.SideSlipLabel->setText(tr("%1").arg(beta));
}

void AirVelocityWidget::setUAS(UASInterface* uas)
{
	disconnect(this,SLOT(UpdateAirSpeed(float)));
	disconnect(this,SLOT(UpdateAoA(float)));
	disconnect(this,SLOT(UpdateSideSlip(float)));
	senseSoarMAV* mav = dynamic_cast<senseSoarMAV*>(uas);
	if(NULL!=mav)
	{
		m_mav = mav;
		connect(m_mav,SIGNAL(AirSpeedChanged(float)),this,SLOT(UpdateAirSpeed(float)));
		connect(m_mav,SIGNAL(AoAChanged(float)),this,SLOT(UpdateAoA(float)));
		connect(m_mav,SIGNAL(SideSlipChanged(float)),this,SLOT(UpdateSideSlip(float)));
	}
}
