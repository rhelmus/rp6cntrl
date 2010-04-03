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
            SLOT(clientHasData(QObject *)));
}

void CTcpServer::clientConnected()
{
    qDebug("Client connected\n");
    
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    
    connect(socket, SIGNAL(disconnected()), disconnectMapper, SLOT(map()));
    disconnectMapper->setMapping(socket, socket);
    
    connect(socket, SIGNAL(readyRead()), clientDataMapper, SLOT(map()));
    clientDataMapper->setMapping(socket, socket);
    
    clientInfo[socket] = 0;
}

void CTcpServer::clientDisconnected(QObject *obj)
{
    clientInfo.remove(qobject_cast<QTcpSocket *>(obj));
    qDebug() << "Client disconnected";
    obj->deleteLater();
}

void CTcpServer::clientHasData(QObject *obj)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(obj);
    qDebug() << "Received client data\n";

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_4_4);
    
    while (true)
    {
        if (clientInfo[socket] == 0)
        {
            if (socket->bytesAvailable() < (int)sizeof(quint32))
                return;
            
            in >> clientInfo[socket];
        }
        
        if (socket->bytesAvailable() < clientInfo[socket])
            return;
        
        clientTcpReceived(in);
        clientInfo[socket] = 0;
    }
}

void CTcpServer::sendText(const QString &key, const QString &text)
{
    for (QMap<QTcpSocket *, quint32>::iterator it=clientInfo.begin();
         it!=clientInfo.end(); ++it)
    {
        CTcpWriter tcpWriter(it.key());
        tcpWriter << key;
        tcpWriter << text;
        tcpWriter.write();
    }
}

void CTcpServer::sendKeyValue(const QString &key, QVariant value)
{
    for (QMap<QTcpSocket *, quint32>::iterator it=clientInfo.begin();
         it!=clientInfo.end(); ++it)
    {
        CTcpWriter tcpWriter(it.key());
        tcpWriter << key;
        tcpWriter << value;
        tcpWriter.write();
    }
}
