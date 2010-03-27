#ifndef CONTROL_H
#define CONTROL_H

#include <QObject>

class CSerialPort;

class CControl: public QObject
{
    Q_OBJECT

    CSerialPort *serialPort;

private slots:
    void beep(void);
    
public:
    CControl(QObject *parent);
};

#endif
