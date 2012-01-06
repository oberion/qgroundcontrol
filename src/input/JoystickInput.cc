/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Joystick interface
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Andreas Romer <mavteam@student.ethz.ch>
 *
 */

#include "JoystickInput.h"

#include <QDebug>
#include <limits.h>
#include "UAS.h"
#include "UASManager.h"
#include "QGC.h"
#include <QMutexLocker>
#include <qfile.h>
#include <qdatastream.h>
#include <qstring.h>

/**
 * The coordinate frame of the joystick axis is the aeronautical frame like shown on this image:
 * @image html http://pixhawk.ethz.ch/wiki/_media/standards/body-frame.png Aeronautical frame
 */
JoystickInput::JoystickInput() :
    defaultIndex(0),
    uas(NULL),
    uasButtonList(QList<int>()),
    done(false),
    thrustAxis(3),
    xAxis(0),
    yAxis(1),
    yawAxis(2),
    joystickName(tr("Uninitialized")),
	m_calibrated(false),
	m_xDir(1),
	m_yDir(1),
	m_yawDir(1),
	m_thrustDir(1)
{
    for (int i = 0; i < 10; i++) {
        calibrationPositive[i] = 32767;
        calibrationNegative[i] = -32768;
    }

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    // Enter main loop
    //start();
}

JoystickInput::~JoystickInput()
{
	{
		QMutexLocker locker(&m_calMutex);
		if(this->isRunning() && m_calibrated)
		{
			QString name(SDL_JoystickName(defaultIndex));
			name.append(".txt");
			QFile out(name);
			if(out.open(QIODevice::WriteOnly))
			{
				QDataStream write(&out);
				{
					QMutexLocker locker(&this->m_axisMutex);
					write << xAxis;
					write << yAxis;
					write << yawAxis;
					write << thrustAxis;
				}
				{
					QMutexLocker locker(&this->m_dirMutex);
					write << m_xDir;
					write << m_yDir;
					write << m_yawDir;
					write << m_thrustDir;
				}
				{
					QMutexLocker locker(&m_calDataMutex);
					for(unsigned int i(0);i<10;i++)
					{
						write << calibrationPositive[i];
						write << calibrationNegative[i];
					}
				}
			}
		}
	}
	{
		QMutexLocker locker(&this->m_doneMutex);
		done = true;
	}
	this->wait();
	this->deleteLater();
}


void JoystickInput::setActiveUAS(UASInterface* uas)
{
    // Only connect / disconnect is the UAS is of a controllable UAS class
    UAS* tmp = 0;
    if (this->uas) {
        tmp = dynamic_cast<UAS*>(this->uas);
        if(tmp) {
            disconnect(this, SIGNAL(joystickChanged(double,double,double,double,int,int)), tmp, SLOT(setManualControlCommands(double,double,double,double)));
            disconnect(this, SIGNAL(buttonPressed(int)), tmp, SLOT(receiveButton(int)));
        }
    }

    this->uas = uas;

    tmp = dynamic_cast<UAS*>(this->uas);
    if(tmp) {
        connect(this, SIGNAL(joystickChanged(double,double,double,double,int,int)), tmp, SLOT(setManualControlCommands(double,double,double,double)));
        connect(this, SIGNAL(buttonPressed(int)), tmp, SLOT(receiveButton(int)));
    }
    if (!isRunning()) {
        start();
    }
}

void JoystickInput::init()
{
    // INITIALIZE SDL Joystick support
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
        printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
    }

    // Enumerate joysticks and select one
    int numJoysticks = SDL_NumJoysticks();

    // Wait for joysticks if none is connected
    while (numJoysticks == 0) {
        MG::SLEEP::msleep(200);
        // INITIALIZE SDL Joystick support
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
            printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
        }
        numJoysticks = SDL_NumJoysticks();
    }

    printf("%d Input devices found:\n", numJoysticks);
    for(int i=0; i < SDL_NumJoysticks(); i++ ) {
        printf("\t- %s\n", SDL_JoystickName(i));
        joystickName = QString(SDL_JoystickName(i));
    }

    printf("\nOpened %s\n", SDL_JoystickName(defaultIndex));

    SDL_JoystickEventState(SDL_ENABLE);

    joystick = SDL_JoystickOpen(defaultIndex);

	QString name(SDL_JoystickName(defaultIndex));
	name.append(".txt");
	QFile in(name);
	if(in.open(QIODevice::ReadOnly))
	{
		QDataStream read(&in);
		{
			read >> xAxis;
			read >> yAxis;
			read >> yawAxis;
			read >> thrustAxis;
		}
		{
			QMutexLocker locker(&this->m_dirMutex);
			read >> m_xDir;
			read >> m_yDir;
			read >> m_yawDir;
			read >> m_thrustDir;
		}
		{
			QMutexLocker locker(&m_calDataMutex);
			for(unsigned int i(0);i<10;i++)
			{
				read >> calibrationPositive[i];
				read >> calibrationNegative[i];
			}
		}
		{
			QMutexLocker locker(&m_calMutex);
			m_calibrated = true;
		}
	}
    // Make sure active UAS is set
    setActiveUAS(UASManager::instance()->getActiveUAS());
}

/**
 * @brief Execute the Joystick process
 */
void JoystickInput::run()
{

    init();

    forever
	{
		{
			QMutexLocker locker(&this->m_doneMutex);
			if(done)
			{
				done = false;
				break;
			}
		}
        while(SDL_PollEvent(&event)) 
		{
            SDL_JoystickUpdate();
			/*
            // Todo check if it would be more beneficial to use the event structure
            switch(event.type) 
			{
            case SDL_KEYDOWN:
                // handle keyboard stuff here
                //qDebug() << "KEY PRESSED!";
                break;

            case SDL_QUIT:
                // Set whatever flags are necessary to 
                // end the main loop here 
                break;

            case SDL_JOYBUTTONDOWN:  // Handle Joystick Button Presses 
                if ( event.jbutton.button == 0 ) {
                    //qDebug() << "BUTTON PRESSED!";
                }
                break;

            case SDL_JOYAXISMOTION:  // Handle Joystick Motion 
                if ( ( event.jaxis.value < -3200 ) || (event.jaxis.value > 3200 ) ) {
                    if( event.jaxis.axis == 0) {
                        // Left-right movement code goes here 
                    }

                    if( event.jaxis.axis == 1) {
                        // Up-Down movement code goes here 
                    }
                }
                break;

            default:
                //qDebug() << "SDL event occured";
                break;
            }*/
        }

        /*// Display all axes
        for(int i = 0; i < SDL_JoystickNumAxes(joystick); i++) {
            //qDebug() << "\rAXIS" << i << "is: " << SDL_JoystickGetAxis(joystick, i);
        }*/
		// THRUST
		double thrustValue;
		{
			QMutexLocker locker(&this->m_axisMutex);
			QMutexLocker locker2(&m_calDataMutex);
			thrustValue = ((double)(SDL_JoystickGetAxis(joystick, thrustAxis) - calibrationNegative[0])) / (calibrationPositive[0] - calibrationNegative[0]);
		}
		{
			QMutexLocker locker(&this->m_dirMutex);
			if(m_thrustDir==-1)
			{
				thrustValue = 1.0 - thrustValue;
			}
		}
		// Bound rounding errors
		if (thrustValue > 1) thrustValue = 1;
		if (thrustValue < 0) thrustValue = 0;
		emit thrustChanged((float)thrustValue);
		// X Axis
		double xValue;
		{
			QMutexLocker locker(&this->m_axisMutex);
			QMutexLocker locker2(&m_calDataMutex);
			xValue = ((double)(SDL_JoystickGetAxis(joystick, xAxis) - calibrationNegative[1])) / (calibrationPositive[1] - calibrationNegative[1]);
		}
		{
			QMutexLocker locker(&this->m_dirMutex);
			xValue = m_xDir*(2.0*xValue - 1.0);
		}
		// Bound rounding errors
		if (xValue > 1.0) xValue = 1.0;
		if (xValue < -1.0) xValue = -1.0;
		emit xChanged((float)xValue);
		// Y Axis
		double yValue;
		{
			QMutexLocker locker(&this->m_axisMutex);
			QMutexLocker locker2(&m_calDataMutex);
			yValue = ((double)(SDL_JoystickGetAxis(joystick, yAxis) - calibrationNegative[2])) / (calibrationPositive[2] - calibrationNegative[2]);
		}
		{
			QMutexLocker locker(&this->m_dirMutex);
			yValue = m_yDir*(2.0*yValue - 1.0);
		}
		// Bound rounding errors
		if (yValue > 1.0) yValue = 1.0;
		if (yValue < -1.0) yValue = -1.0;
		emit yChanged((float)yValue);
		// Yaw Axis
		double yawValue;
		{
			QMutexLocker locker(&this->m_axisMutex);
			QMutexLocker locker2(&m_calDataMutex);
			yawValue = ((double)(SDL_JoystickGetAxis(joystick, yawAxis) - calibrationNegative[3])) / (calibrationPositive[3] - calibrationNegative[3]);
		}
		{
			QMutexLocker locker(&this->m_dirMutex);
			yawValue = m_yawDir*(2.0*yawValue - 1.0);
		}
		// Bound rounding errors
		if (yawValue > 1.0f) yawValue = 1.0f;
		if (yawValue < -1.0f) yawValue = -1.0f;
		emit yawChanged((float)yawValue);
		// Get joystick hat position, convert it to vector
		int hatPosition = SDL_JoystickGetHat(joystick, 0);
		int xHat,yHat;
		xHat = 0;
		yHat = 0;
		// Build up vectors describing the hat position
		//
		// Coordinate frame for joystick hat:
		//
		//    y
		//    ^
		//    |
		//    |
		//    0 ----> x
		//
		if ((SDL_HAT_UP & hatPosition) > 0) yHat = 1;
		if ((SDL_HAT_DOWN & hatPosition) > 0) yHat = -1;
		if ((SDL_HAT_LEFT & hatPosition) > 0) xHat = -1;
		if ((SDL_HAT_RIGHT & hatPosition) > 0) xHat = 1;
		// Send new values to rest of groundstation
		emit hatDirectionChanged(xHat, yHat);
		{
			QMutexLocker locker(&m_calMutex);
			if(m_calibrated)
			{
				emit joystickChanged(yValue, xValue, yawValue, thrustValue, xHat, yHat);
			}
			else
			{
				qDebug() << "Joystick not calibrated. No commands are sent to UAS";
			}
		}

		// Display all buttons
        for(int i = 0; i < SDL_JoystickNumButtons(joystick); i++) {
            //qDebug() << "BUTTON" << i << "is: " << SDL_JoystickGetAxis(joystick, i);
            if(SDL_JoystickGetButton(joystick, i)) {
                emit buttonPressed(i);
                // Check if button is a UAS select button

                if (uasButtonList.contains(i)) {
                    UASInterface* uas = UASManager::instance()->getUASForId(i);
                    if (uas) {
                        UASManager::instance()->setActiveUAS(uas);
                    }
                }
            }

        }

        // Sleep, update rate of joystick is approx. 50 Hz (1000 ms / 50 = 20 ms)
		this->msleep(20);
    }

}

const QString& JoystickInput::getName()
{
    return joystickName;
}

void JoystickInput::newRollAxis(int rollAxis)
{
	QMutexLocker locker(&this->m_axisMutex);
	yAxis = rollAxis-1;
	return;
}

void JoystickInput::newPitchAxis(int pitchAxis)
{
	QMutexLocker locker(&this->m_axisMutex);
	xAxis = pitchAxis-1;
	return;
}

void JoystickInput::newYawAxis(int newYawAxis)
{
	QMutexLocker locker(&this->m_axisMutex);
	yawAxis = newYawAxis-1;
	return;
}

void JoystickInput::newThrustAxis(int newThrustAxis)
{
	QMutexLocker locker(&this->m_axisMutex);
	thrustAxis = newThrustAxis-1;
	return;
}

void JoystickInput::newRollDir(int rollDir)
{
	QMutexLocker locker(&this->m_dirMutex);
	m_yDir = -1*(rollDir - 1);
	return;
}

void JoystickInput::newPitchDir(int pitchDir)
{
	QMutexLocker locker(&this->m_dirMutex);
	m_xDir = -1*(pitchDir - 1);
	return;
}

void JoystickInput::newYawDir(int yawDir)
{
	QMutexLocker locker(&this->m_dirMutex);
	m_yawDir = -1*(yawDir - 1);
	return;
}

void JoystickInput::newThrustDir(int thrustDir)
{
	QMutexLocker locker(&this->m_dirMutex);
	m_thrustDir = -1*(thrustDir - 1);
	return;
}

void JoystickInput::getInitCalibData(int& rollInput, int& pitchInput, int& yawInput, int& thrustInput, int& rollInv, int& pitchInv, int& yawInv, int& thrustInv)
{
	{
		QMutexLocker locker(&this->m_axisMutex);
		rollInput = yAxis+1;
		pitchInput = xAxis+1;
		yawInput = yawAxis+1;
		thrustInput = thrustAxis+1;
	}
	{
		QMutexLocker locker(&this->m_dirMutex);
		rollInv = m_yDir;
		pitchInv = m_xDir;
		yawInv = m_yawDir;
		thrustInv = m_thrustDir;
	}
	return;
}

bool JoystickInput::getCalibState(void)
{
	QMutexLocker locker(&m_calMutex);
	return m_calibrated;
}

void JoystickInput::setCalibData(int16_t *calibPositive, int16_t *calibNegative)
{
	QMutexLocker locker2(&m_calDataMutex);
	for (int i = 0; i < 10; i++) 
	{
        calibrationPositive[i] = *(calibPositive+i);
        calibrationNegative[i] = *(calibNegative+i);
    }
	QMutexLocker locker(&m_calMutex);
	m_calibrated = true;
}

void JoystickInput::getValues(int16_t &xValue, int16_t &yValue, int16_t &yawValue, int16_t &thrustValue)
{
	xValue = SDL_JoystickGetAxis(joystick, xAxis);
	yValue = SDL_JoystickGetAxis(joystick, yAxis);
	yawValue = SDL_JoystickGetAxis(joystick, yawAxis);
	thrustValue = SDL_JoystickGetAxis(joystick, thrustAxis);
}