#include <QObject>
#include <QTcpSocket>

#include "tui/window.h"

class QAbstractSocket;
class QDataStream;

namespace NNCurses {
class CButton;
class CLabel;
}

class CNCursManager: public QObject
{
    Q_OBJECT
    
private slots:
    void updateNCurs(void);
    void stopNCurs(void);
    
public:
    CNCursManager(QObject *parent);
};

// QObject must be inherited first!
class CNCursClient: public QObject, public NNCurses::CWindow
{
    Q_OBJECT
    
    QTcpSocket *clientSocket;
    quint32 tcpReadBlockSize;
    NNCurses::CButton *connectButton;
    
    typedef std::map<std::string, NNCurses::CLabel *> TDisplayMap;
    TDisplayMap movementDisplays, sensorDisplays, otherDisplays;
    
    void initDisplayMaps(void);
    NNCurses::CBox *createDisplayWidget(const std::string &title,
                                        TDisplayMap &map);
    void updateConnection(bool connected);
    void parseTcp(QDataStream &stream);

private slots:
    void connectedToServer(void);
    void disconnectedFromServer(void);
    void serverHasData(void);
    void socketError(QAbstractSocket::SocketError error);

protected:
    virtual bool CoreHandleEvent(NNCurses::CWidget *emitter, int type);
    
public:
    CNCursClient(void);
};
