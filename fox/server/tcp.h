#ifndef TCP_H
#define TCP_H

#include <stdint.h>

#include <QMap>
#include <QObject>

class QSignalMapper;
class QTcpServer;
class QTcpSocket;

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

    void sendText(const QString &key, const QString &text);
    void sendKeyValue(const QString &key, QVariant value);

signals:
    void clientTcpReceived(QDataStream &stream);
};

#endif
