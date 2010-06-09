#include <QObject>
#include <QTcpSocket>

#include "shared.h"
#include "tui/window.h"

class QAbstractSocket;
class QDataStream;

namespace NNCurses {
class CButton;
class CLabel;
class CTextField;
}

class CDisplayWidget;

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

    enum EDisplayType
    {
        DISPLAY_SPEED=0,
        DISPLAY_DISTANCE,
        DISPLAY_CURRENT,
        DISPLAY_DIRECTION,

        DISPLAY_LIGHT,
        DISPLAY_ACS,
        DISPLAY_BUMPERS,

        DISPLAY_BATTERY,
        DISPLAY_RC5,
        DISPLAY_MAIN_LEDS,
        DISPLAY_M32_LEDS,
        DISPLAY_KEYS,

        DISPLAY_MAX_INDEX
    };
    
    typedef std::vector<NNCurses::CLabel *> TDisplayList;
    typedef NNCurses::CBox TScreen;
    typedef std::vector<TScreen *> TScreenList;

    QTcpSocket *clientSocket;
    quint32 tcpReadBlockSize;

    TScreenList screenList;
    TScreen *activeScreen;
    TScreen *mainScreen;
    TScreen *logScreen;
    
    NNCurses::CButton *connectButton;
    TDisplayList dataDisplays;

    NNCurses::CTextField *logWidget;
    
    TScreen *createMainScreen(void);
    TScreen *createLogScreen(void);

    void enableScreen(TScreen *screen);
    void appendLogText(std::string text);
    CDisplayWidget *createMovementDisplay(void);
    CDisplayWidget *createSensorDisplay(void);
    CDisplayWidget *createOtherDisplay(void);
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
