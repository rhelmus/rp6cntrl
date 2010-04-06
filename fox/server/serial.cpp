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
    int index = msgBuffer.indexOf(SERIAL_MSG_START);

    if (index == -1)
    {
        // Only raw text
        textBuffer += msgBuffer;
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
            assert(msgsize <= 3);
            
            if ((nextbytes-1) < msgsize)
                break; // Wait for more bytes

//             qDebug() << QString("Serial MSG: %1 (%2 bytes)").arg((int)msgBuffer[2]).arg(msgsize);
            emit msgAvailable(msgBuffer.mid(2, msgsize));
            msgBuffer.remove(0, 2 + msgsize);
            
            index = msgBuffer.indexOf(SERIAL_MSG_START);
        }
    }

    index = textBuffer.indexOf('\n');
    while (index != -1)
    {
        emit textAvailable(textBuffer.left(index)); // Emit without newline
        textBuffer.remove(0, index+1);
        index = textBuffer.indexOf('\n');
    }
    
    /*
    int end = buffer.indexOf('\n');
    while (end != -1)
    {
        emit textAvailable(buffer.left(end)); // Emit without newline
        buffer.remove(0, end+1);
        end = buffer.indexOf('\n');
    }*/
/*    if (buffer.contains('\n'))
    {
        foreach(QByteArray line, buffer.split('\n'))
        {
            if (line.size() && line.contains('\n'))
                emit textAvailable(line);
        }

        int nl = buffer.lastIndexOf('\n');
        if (nl != -1)
            buffer.remove(0, nl+1);
    }*/
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
