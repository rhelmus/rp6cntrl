#include <QObject>
#include <QTcpSocket>
#include <QTime>

#include "client_base.h"
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
class CNCursClient;

class CNCursManager: public QObject
{
    Q_OBJECT
    
    CNCursClient *NCursClient;

private slots:
    void updateNCurs(void);
    void stopNCurs(void);
    
public:
    CNCursManager(QObject *parent);
};

// QObject derived classes must be inherited first!
class CNCursClient: public CBaseClient, public NNCurses::CWindow
{
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
    
    typedef NNCurses::CBox TScreen;
    typedef std::vector<TScreen *> TScreenList;

    TScreenList screenList;
    TScreen *activeScreen;
    TScreen *mainScreen, *consoleScreen, *logScreen;
    bool drivingEnabled;

    struct SSensorData
    {
        uint32_t total, count;
        SSensorData(void) : total(0), count(0) {}
    };

    std::map<ETcpMessage, SSensorData> averagedSensorDataMap;

    CDisplayWidget *movementDisplay, *sensorDisplay, *otherDisplay;
    NNCurses::CButton *connectButton;

    NNCurses::CTextField *consoleOutput, *logWidget;

    QTime updateTime;
    
    TScreen *createMainScreen(void);
    TScreen *createConsoleScreen(void);
    TScreen *createLogScreen(void);

    void enableScreen(TScreen *screen);
    CDisplayWidget *createMovementDisplay(void);
    CDisplayWidget *createSensorDisplay(void);
    CDisplayWidget *createOtherDisplay(void);

    virtual void updateConnection(bool connected);
    virtual void tcpError(const QString &) {}
    virtual void tcpRobotStateUpdate(const SStateSensors &,
                                     const SStateSensors &newstate);
    virtual void tcpHandleRobotData(ETcpMessage msg, int data);
    virtual void updateDriveSpeed(int, int) {}
    
protected:
    virtual void appendConsoleOutput(const QString &text);
    virtual void appendLogOutput(const QString &text);
    
    virtual bool CoreHandleEvent(NNCurses::CWidget *emitter, int type);
    virtual bool CoreHandleKey(wchar_t key);
    virtual void CoreGetButtonDescs(NNCurses::TButtonDescList &list);
    
public:
    CNCursClient(void);

    void update(void);
};
