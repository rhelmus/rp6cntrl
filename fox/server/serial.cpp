#include <assert.h>

#include <QtCore>

#include "qextserialport.h"
#include "serial.h"

CSerialPort::CSerialPort(QObject *parent) : QObject(parent)
{
    serialPort = new QextSerialPort("/dev/ttyUSB0", QextSerialPort::EventDriven);
    serialPort->setParent(this);
    serialPort->setBaudRate(BAUD38400);
    serialPort->setFlowControl(FLOW_OFF);
    serialPort->setParity(PAR_NONE);
    serialPort->setDataBits(DATA_8);
    serialPort->setStopBits(STOP_1);
    
    if (serialPort->open(QIODevice::ReadWrite) == true)
    {
        connect(serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        //        connect(port, SIGNAL(dsrChanged(bool)), this, SLOT(onDsrChanged(bool)));
        if (!(serialPort->lineStatus() & LS_DSR))
            qDebug() << "warning: device is not turned on";
        qDebug() << "listening for data on" << serialPort->portName();
    }
    else
        qDebug() << "device failed to open:" << serialPort->errorString();
    
    serialPort->setDtr(false);
    serialPort->setRts(false);
}

void CSerialPort::onReadyRead()
{
    QByteArray bytes;
    int a = serialPort->bytesAvailable();
    bytes.resize(a);
    if (serialPort->read(bytes.data(), bytes.size()) == -1)
        return;

    // For easy msg parsing
    msgBuffer += bytes;

    // Format:
    //  0: Start marker
    //  1: Msg size
    //  2: Msg type
    //  n: Data
    int index = msgBuffer.indexOf(SERIAL_MSG_START);

    if (index == -1)
    {
        // Only raw text
        textBuffer += msgBuffer;
        msgBuffer.clear();
    }
    else
    {
        while (index != -1)
        {
            if (index > 0) // Text in between?
            {
                // Raw text
                textBuffer += msgBuffer.left(index);
                msgBuffer.remove(0, index);
            }

            int nextbytes = msgBuffer.size() - 1;
            if (nextbytes < 1)
                break; // Wait for more bytes
                
            int msgsize = msgBuffer[1];
            
            if ((nextbytes-1) < msgsize)
                break; // Wait for more bytes

            emit msgAvailable(static_cast<ESerialMessage>((int)msgBuffer[2]),
                              msgBuffer.mid(3, msgsize));
            msgBuffer.remove(0, 2 + msgsize);
            
            index = msgBuffer.indexOf(SERIAL_MSG_START);

            // Add any remaining text to raw text buffer
            if (index == -1)
            {
                textBuffer += msgBuffer;
                msgBuffer.clear();
            }
        }
    }

    index = textBuffer.indexOf('\n');
    while (index != -1)
    {
        emit textAvailable(textBuffer.left(index)); // Emit without newline
        textBuffer.remove(0, index+1);
        index = textBuffer.indexOf('\n');
    }
}

void CSerialPort::disableRTS()
{
    serialPort->setRts(false);
}

void CSerialPort::resetRP6()
{
    serialPort->setRts(true);
    QTimer::singleShot(100, this, SLOT(disableRTS));
}

void CSerialPort::sendCommand(const QString &command)
{
    serialPort->write((command + "\n").toLatin1());
}
