#include <QtCore>

#include "serial.h"
#include "server.h"

CControl::CControl(QObject *parent) : QObject(parent)
{
    serialPort = new CSerialPort(this);
    serialPort->launchRP6();

    // Test
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(beep()));
    timer->start(1000);
}

void CControl::beep()
{
    serialPort->sendCommand("beep 100 10");
}
