#include <QObject>
#include <QStringList>
#include <QTcpSocket>

#include "shared.h"

class QDataStream;

class CBaseClient;

// HACK: Class is split in 2 to circumvent MI with QObject
class CBaseClientTcpHandler: public QObject
{
    Q_OBJECT
    
    CBaseClient *baseClient;
    quint32 tcpReadBlockSize;
    QTcpSocket *clientSocket;
   
private slots:
    void connectedToServer(void);
    void disconnectedFromServer(void);
    void serverHasData(void);
    void socketError(QAbstractSocket::SocketError);
    
public:
    CBaseClientTcpHandler(CBaseClient *b);

    QTcpSocket *getSocket(void) { return clientSocket; }
    void connectToHost(const QString &host);
    void disconnectFromServer(void) { clientSocket->abort(); }
    bool connected(void) const
    { return (clientSocket->state() == QAbstractSocket::ConnectedState); }
};

class CBaseClient
{
    CBaseClientTcpHandler tcpHandler;
    SStateSensors currentStateSensors;
    SMotorDirections currentMotorDirections;
    int driveForwardSpeed, driveTurnSpeed;
    bool driveTurning;

    void parseTcp(QDataStream &stream);
    void updateMotorDirections(const SMotorDirections &dir);
    void changeDriveSpeedVar(int &speed, int delta);

    virtual void updateConnection(bool connected) = 0;
    virtual void tcpError(const QString &error) = 0;
    virtual void tcpRobotStateUpdate(const SStateSensors &oldstate,
                                     const SStateSensors &newstate) = 0;
    virtual void tcpHandleRobotData(ETcpMessage msg, int data) = 0;
    virtual void tcpLuaScripts(const QStringList &) { }
    virtual void tcpRequestedScript(const QByteArray &) { }
    virtual void tcpScriptRunning(bool) { }
    virtual void tcpHandleLuaMsg(const QString &, QDataStream &) { }
    virtual void updateDriveSpeed(int left, int right) = 0;

    friend class CBaseClientTcpHandler;
    
protected:
    void connectToHost(const QString &host) { tcpHandler.connectToHost(host); }
    void disconnectFromServer(void) { tcpHandler.disconnectFromServer(); }
    bool connected(void) const { return tcpHandler.connected(); }
    void executeCommand(const QString &cmd);
    void updateDriving(int dir);
    void stopDrive(void);
    void uploadLocalScript(const QString &name, const QByteArray &text);
    void runLocalScript(const QByteArray &text);
    void uploadRunLocalScript(const QString &name, const QByteArray &text);
    void runServerScript(const QString &name);
    void removeServerScript(const QString &name);
    void downloadServerScript(const QString &name);
    void executeScriptCommand(const QString &cmd,
                              const QStringList &args=QStringList());

    virtual void appendConsoleOutput(const QString &text) = 0;
    virtual void appendLogOutput(const QString &text) = 0;
    virtual void appendLuaOutput(const QString &text) { appendLogOutput(QString("[Lua]: %1\n").arg(text)); }

public:
    CBaseClient(void);
    virtual ~CBaseClient(void) {}
};

