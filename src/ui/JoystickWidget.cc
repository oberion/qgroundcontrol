#include "JoystickWidget.h"
#include "ui_JoystickWidget.h"
#include <QDebug>
#include <qmessagebox.h>

JoystickWidget::JoystickWidget(JoystickInput* joystick, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::JoystickWidget)
{
    m_ui->setupUi(this);
    this->joystick = joystick;
	QGC::SLEEP::msleep(20); // Let the thread sleep for some millisecond in order to be sure that joystick thread has initialized itself TO DO: better solution
	int rInp, pInp, yInp, tInp, rInv, pInv, yInv, tInv;
	joystick->getInitCalibData(rInp,pInp,yInp,tInp,rInv,pInv,yInv,tInv);
	m_ui->rollInput->setValue(rInp);
	m_ui->pitchInput->setValue(pInp);
	m_ui->yawInput->setValue(yInp);
	m_ui->thrustInput->setValue(tInp);
	if(-1==rInv)
	{
		m_ui->rollInvert->setChecked(true);
	}
	else
	{
		m_ui->rollInvert->setChecked(false);
	}
	if(-1==pInv)
	{
		m_ui->pitchInvert->setChecked(true);
	}
	else
	{
		m_ui->pitchInvert->setChecked(false);
	}
	if(-1==yInv)
	{
		m_ui->yawInvert->setChecked(true);
	}
	else
	{
		m_ui->yawInvert->setChecked(false);
	}
	if(-1==tInv)
	{
		m_ui->thrustInvert->setChecked(true);
	}
	else
	{
		m_ui->thrustInvert->setChecked(false);
	}
	if(joystick->getCalibState())
	{
		m_ui->CalibLabel->setText("Already Calibrated");
	}

    //connect(this->joystick, SIGNAL(joystickChanged(double,double,double,double,int,int)), this, SLOT(updateJoystick(double,double,double,double,int,int)));
	connect(this->joystick, SIGNAL(xChanged(float)),this,SLOT(setX(float)));
	connect(this->joystick, SIGNAL(yChanged(float)),this,SLOT(setY(float)));
	connect(this->joystick, SIGNAL(yawChanged(float)),this,SLOT(setZ(float)));
	connect(this->joystick, SIGNAL(thrustChanged(float)),this,SLOT(setThrottle(float)));
	connect(this->joystick, SIGNAL(hatDirectionChanged(int, int)),this,SLOT(setHat(int, int)));
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(pressKey(int)));
	connect(m_ui->rollInput, SIGNAL(valueChanged(int)), this->joystick, SLOT(newRollAxis(int)));
	connect(m_ui->pitchInput, SIGNAL(valueChanged(int)), this->joystick, SLOT(newPitchAxis(int)));
	connect(m_ui->yawInput, SIGNAL(valueChanged(int)), this->joystick, SLOT(newYawAxis(int)));
	connect(m_ui->thrustInput, SIGNAL(valueChanged(int)), this->joystick, SLOT(newThrustAxis(int)));
	connect(m_ui->rollInvert, SIGNAL(stateChanged(int)), this->joystick, SLOT(newRollDir(int)));
	connect(m_ui->pitchInvert, SIGNAL(stateChanged(int)), this->joystick, SLOT(newPitchDir(int)));
	connect(m_ui->yawInvert, SIGNAL(stateChanged(int)), this->joystick, SLOT(newYawDir(int)));
	connect(m_ui->thrustInvert, SIGNAL(stateChanged(int)), this->joystick, SLOT(newThrustDir(int)));
	connect(m_ui->closeButton, SIGNAL(pressed()), this, SLOT(close()));
	connect(m_ui->calibButton, SIGNAL(pressed()), this, SLOT(calibration()));

    // Display the widget
    this->window()->setWindowTitle(tr("Joystick Settings"));
    if (joystick) updateStatus(tr("Found joystick: %1").arg(joystick->getName()));

    this->show();
}

JoystickWidget::~JoystickWidget()
{
    delete m_ui;
}

void JoystickWidget::updateJoystick(double roll, double pitch, double yaw, double thrust, int xHat, int yHat)
{
    setX(pitch);
    setY(roll);
    setZ(yaw);
    setThrottle(thrust);
    setHat(xHat, yHat);
}

void JoystickWidget::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void JoystickWidget::setThrottle(float thrust)
{
    m_ui->thrust->setValue(thrust*100);
}

void JoystickWidget::setX(float x)
{
    m_ui->xSlider->setValue(x*100);
    m_ui->xValue->display(x*100);
}

void JoystickWidget::setY(float y)
{
    m_ui->ySlider->setValue(y*100);
    m_ui->yValue->display(y*100);
}

void JoystickWidget::setZ(float z)
{
    m_ui->dial->setValue(z*100);
}

void JoystickWidget::setHat(int x, int y)
{
	updateStatus(tr("Hat position: x: %1, y: %2").arg(x).arg(y));
}

void JoystickWidget::clearKeys()
{
    QString colorstyle;
    QColor buttonStyleColor = QColor(200, 20, 20);
    colorstyle = QString("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: %1;}").arg(buttonStyleColor.name());

    m_ui->buttonLabel0->setStyleSheet(colorstyle);
    m_ui->buttonLabel1->setStyleSheet(colorstyle);
    m_ui->buttonLabel2->setStyleSheet(colorstyle);
    m_ui->buttonLabel3->setStyleSheet(colorstyle);
    m_ui->buttonLabel4->setStyleSheet(colorstyle);
    m_ui->buttonLabel5->setStyleSheet(colorstyle);
    m_ui->buttonLabel6->setStyleSheet(colorstyle);
    m_ui->buttonLabel7->setStyleSheet(colorstyle);
    m_ui->buttonLabel8->setStyleSheet(colorstyle);
    m_ui->buttonLabel9->setStyleSheet(colorstyle);
}

void JoystickWidget::pressKey(int key)
{
    QString colorstyle;
    QColor buttonStyleColor = QColor(20, 200, 20);
    colorstyle = QString("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: %1;}").arg(buttonStyleColor.name());
    switch(key) {
    case 0:
        m_ui->buttonLabel0->setStyleSheet(colorstyle);
        break;
    case 1:
        m_ui->buttonLabel1->setStyleSheet(colorstyle);
        break;
    case 2:
        m_ui->buttonLabel2->setStyleSheet(colorstyle);
        break;
    case 3:
        m_ui->buttonLabel3->setStyleSheet(colorstyle);
        break;
    case 4:
        m_ui->buttonLabel4->setStyleSheet(colorstyle);
        break;
    case 5:
        m_ui->buttonLabel5->setStyleSheet(colorstyle);
        break;
    case 6:
        m_ui->buttonLabel6->setStyleSheet(colorstyle);
        break;
    case 7:
        m_ui->buttonLabel7->setStyleSheet(colorstyle);
        break;
    case 8:
        m_ui->buttonLabel8->setStyleSheet(colorstyle);
        break;
    case 9:
        m_ui->buttonLabel9->setStyleSheet(colorstyle);
        break;
    }
    QTimer::singleShot(20, this, SLOT(clearKeys()));
    updateStatus(tr("Key %1 pressed").arg(key));
}

void JoystickWidget::updateStatus(const QString& status)
{
    m_ui->statusLabel->setText(status);
}

void JoystickWidget::calibration(void)
{
	// change text status message
	m_ui->CalibLabel->setText("Already Calibrated");
	int16_t CalibPositive[10];
	int16_t CalibNegative[10];
	// initializing
	for(unsigned int i(0);i<10;i++)
	{
		CalibPositive[i] = -32768;
		CalibNegative[i] = 32767;
	}
	QMessageBox msgBox;
	msgBox.setText(tr("Start calibration procedure"));
	msgBox.setInformativeText(tr("After pressing the ok button move the Joystick to all limits for the next 10 seconds"));
	msgBox.exec();
	for(unsigned int i(0);i<500;i++)
	{
		int16_t xValue, yValue, yawValue, thrustValue;
		joystick->getValues(xValue, yValue, yawValue, thrustValue);
		if(thrustValue>CalibPositive[0])
		{
			CalibPositive[0]=thrustValue;
		}
		if(thrustValue<CalibNegative[0])
		{
			CalibNegative[0]=thrustValue;
		}
		if(xValue>CalibPositive[1])
		{
			CalibPositive[1]=xValue;
		}
		if(xValue<CalibNegative[1])
		{
			CalibNegative[1]=xValue;
		}
		if(yValue>CalibPositive[2])
		{
			CalibPositive[2]=yValue;
		}
		if(yValue<CalibNegative[2])
		{
			CalibNegative[2]=yValue;
		}
		if(yawValue>CalibPositive[3])
		{
			CalibPositive[3]=yawValue;
		}
		if(yawValue<CalibNegative[3])
		{
			CalibNegative[3]=yawValue;
		}
		QGC::SLEEP::msleep(20);
	}
	QMessageBox msgBox2;
	msgBox2.setText(tr("Calibration completed"));
	msgBox2.exec();
	// Sent calibration data to joystickInput class
	joystick->setCalibData(CalibPositive,CalibNegative);
}