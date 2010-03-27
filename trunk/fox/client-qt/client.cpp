#include <QtGui>
#include <QtNetwork>

#include "client.h"

CQtClient::CQtClient() : tcpReadBlockSize(0)
{
    QWidget *cw = new QWidget;
    setCentralWidget(cw);
    
    QVBoxLayout *vbox = new QVBoxLayout(cw);
    
    vbox->addWidget(serverEdit = new QLineEdit);
    
    QPushButton *button = new QPushButton("Connect");
    connect(button, SIGNAL(clicked()), this, SLOT(connectToServer()));
    vbox->addWidget(button);
    
    vbox->addWidget(logWidget = new QPlainTextEdit);
    logWidget->setReadOnly(true);
    logWidget->setCenterOnScroll(true);
    
    clientSocket = new QTcpSocket(this);
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
}

void CQtClient::appendLogText(const QString &text)
{
    QTextCursor cur = logWidget->textCursor();
    cur.movePosition(QTextCursor::End);
    logWidget->setTextCursor(cur);
    logWidget->insertPlainText(text);
}

void CQtClient::serverHasData()
{
    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_4_4);

    while (true)
    {
        if (tcpReadBlockSize == 0)
        {
            if (clientSocket->bytesAvailable() < (int)sizeof(quint32))
                return;

            in >> tcpReadBlockSize;
        }

        if (clientSocket->bytesAvailable() < tcpReadBlockSize)
            return;

        QString key, text;
        in >> key >> text;
        appendLogText(QString("Key: %1\nText: %2\n").arg(key).arg(text));
//         parseTcp(in);
        tcpReadBlockSize = 0;
    }
}

void CQtClient::socketError(QAbstractSocket::SocketError error)
{
    QMessageBox::critical(this, "Socket error", clientSocket->errorString());
}

void CQtClient::connectToServer()
{
    clientSocket->abort();
    clientSocket->connectToHost(serverEdit->text(), 40000);
}
