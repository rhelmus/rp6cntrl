#ifndef CONTROL_H
#define CONTROL_H

#include <QMap>
#include <QObject>

#include "shared.h"

class CSerialPort;
class CTcpServer;

class CControl: public QObject
{
    Q_OBJECT

    enum EDataSerialType { DATA_BYTE, DATA_WORD, DATA_ACD };
    
    struct SSerial2TcpInfo
    {
        EDataSerialType dataType;
        const char *tcpVariable;
        SSerial2TcpInfo(void) {}
        SSerial2TcpInfo(EDataSerialType d,
                        const char *t) : dataType(d), tcpVariable(t) {}
    };
    
    CSerialPort *serialPort;
    CTcpServer *tcpServer;
    QMap<ESerialMessage, SSerial2TcpInfo> serial2TcpMap;

    void initSerial2TcpMap(void);
    void sendSerial2Tcp(ESerialMessage msg, const QByteArray &serialdata);

private slots:
    void handleSerialText(const QByteArray &text);
    void handleSerialMSG(const QByteArray &text);
    void parseClientTcp(QDataStream &stream);
    void enableRP6Slave(void);
    
public:
    CControl(QObject *parent);
};

#endif
