#include <QtCore>
#include <QtNetwork>
#include <iostream>

#include "tcputil.h"
#include "tcp.h"

using std::cerr;

CTcpServer::CTcpServer(QObject *parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, 40000))
    {
        cerr << "Failed to start server:" << tcpServer->errorString().toStdString();
        QCoreApplication::exit(1);
        return;
    }
    
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(clientConnected()));
    
    disconnectMapper = new QSignalMapper(this);
    connect(disconnectMapper, SIGNAL(mapped(QObject *)), this,
            SLOT(clientDisconnected(QObject *)));

    clientDataMapper = new QSignalMapper(this);
    connect(clientDataMapper, SIGNAL(mapped(QObject *)), this,
            SLOT(clientDisconnected(QObject *)));    
}

void CTcpServer::clientConnected()
{
    qDebug("Client connected\n");
    
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), disconnectMapper, SLOT(map()));
    connect(socket, SIGNAL(readyRead()), clientDataMapper, SLOT(map()));
    clients << socket;
}

void CTcpServer::clientDisconnected(QObject *obj)
{
    clients.removeOne(qobject_cast<QTcpSocket *>(obj));
    obj->deleteLater();
}

void CTcpServer::clientHasData(QObject *obj)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(obj);
    qDebug() << "Received client data\n";
}

void CTcpServer::sendText(const QString &key, const QString &text)
{
    foreach(QTcpSocket *client, clients)
    {
        CTcpWriter tcpWriter(client);
        tcpWriter << key;
        tcpWriter << text;
        tcpWriter.write();
    }
}
