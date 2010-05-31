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

    typedef std::map<std::string, NNCurses::CLabel *> TDisplayMap;
    typedef NNCurses::CBox TScreen;
    typedef std::vector<TScreen *> TScreenList;

    QTcpSocket *clientSocket;
    quint32 tcpReadBlockSize;

    TScreenList screenList;
    TScreen *activeScreen;
    TScreen *mainScreen;
    TScreen *logScreen;
    
    NNCurses::CButton *connectButton;
    TDisplayMap movementDisplays, sensorDisplays, otherDisplays;

    TScreen *createMainScreen(void);
    TScreen *createLogScreen(void);

    void enableScreen(TScreen *screen);
    void initDisplayMaps(void);
    NNCurses::CBox *createDisplayWidget(const std::string &title, TDisplayMap &map);
    void updateConnection(bool connected);
    void parseTcp(QDataStream &stream);

private slots:
    void connectedToServer(void);
    void disconnectedFromServer(void);
    void serverHasData(void);
    void socketError(QAbstractSocket::SocketError error);

protected:
    virtual bool CoreHandleEvent(NNCurses::CWidget *emitter, int type);
    virtual bool CoreHandleKey(wchar_t key);
    virtual void CoreGetButtonDescs(NNCurses::TButtonDescList &list);
    
public:
    CNCursClient(void);
};
