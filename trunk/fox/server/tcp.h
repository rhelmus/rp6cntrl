#ifndef TCP_H
#define TCP_H

#include <QList>
#include <QObject>

class QSignalMapper;
class QTcpServer;
class QTcpSocket;

class CTcpServer: public QObject
{
    Q_OBJECT

    QTcpServer *tcpServer;
    QSignalMapper *disconnectMapper, *clientDataMapper;
    QList<QTcpSocket *> clients;

private slots:
    void clientConnected(void);
    void clientDisconnected(QObject *obj);
    void clientHasData(QObject *obj);

public:
    CTcpServer(QObject *parent);

    void sendText(const QString &key, const QString &text);
};

#endif
