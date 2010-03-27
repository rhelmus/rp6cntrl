#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>

class QLineEdit;
class QPlainTextEdit;

class CQtClient: public QMainWindow
{
    Q_OBJECT
    
    quint32 tcpReadBlockSize;
    QTcpSocket *clientSocket;
    QLineEdit *serverEdit;
    QPlainTextEdit *logWidget;
    
    void appendLogText(const QString &text);
    
private slots:
    void serverHasData(void);
    void socketError(QAbstractSocket::SocketError error);
    void connectToServer(void);
    
public:
    CQtClient(void);
};

#endif
