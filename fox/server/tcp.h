#ifndef TCP_H
#define TCP_H

#include <stdint.h>

#include <QMap>
#include <QObject>
#include <QTcpSocket>

#include "shared.h"
#include "tcputil.h"

class QSignalMapper;
class QTcpServer;

class CTcpServer: public QObject
{
    Q_OBJECT

    QTcpServer *tcpServer;
    QSignalMapper *disconnectMapper, *clientDataMapper;
    QMap<QTcpSocket *, quint32> clientInfo;

private slots:
    void clientConnected(void);
    void clientDisconnected(QObject *obj);
    void clientHasData(QObject *obj);

public:
    CTcpServer(QObject *parent);

    template <typename C> void send(ETcpMessage msg, const C &value)
    {
        for (QMap<QTcpSocket *, quint32>::iterator it=clientInfo.begin();
            it!=clientInfo.end(); ++it)
        {
            CTcpWriter tcpWriter(it.key());
            tcpWriter << static_cast<uint8_t>(msg);
            tcpWriter << value;
            tcpWriter.write();
        }
    }

signals:
    void clientTcpReceived(QDataStream &stream);
};

#endif
