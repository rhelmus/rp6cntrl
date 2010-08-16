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
    void send(const QByteArray &by);

    // Convenience function
    template <typename C> void send(ETcpMessage msg, const C &value)
    {
        send(CTcpMsgComposer(msg) << value);
    }

    bool hasConnections(void) const { return !clientInfo.isEmpty(); }

signals:
    void newConnection(void);
    void clientTcpReceived(QDataStream &stream);
};

#endif
