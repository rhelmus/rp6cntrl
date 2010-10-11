#ifndef CONTROL_H
#define CONTROL_H

#include <QMap>
#include <QObject>

#include "lua.h"
#include "shared.h"

class CSerialPort;
class CTcpServer;
class QTimer;

class CControl: public QObject
{
    Q_OBJECT
  
    struct SSerial2TcpInfo
    {
        ETcpMessage tcpMessage;
        const char *luaFunc;
        SSerial2TcpInfo(void) {}
        SSerial2TcpInfo(ETcpMessage m, const char *l) : tcpMessage(m), luaFunc(l) {}
    };

    typedef QMap<QString, QVariant> TLuaScriptMap;
    typedef QMap<ESerialMessage, SSerial2TcpInfo> TSerial2TcpMap;

    class CTcpInfo
    {
        int32_t total, count, latest;

    public:
        CTcpInfo(void) : total(0), count(0), latest(0) { }

        int32_t data(void) const
        {
            if (count > 0)
                return total / count;
            return latest;
        }

        int32_t latestData(void) const { return latest; }

        void addData(int32_t data)
        {
            total += data;
            ++count;
            latest = data;
        }

        void setData(int32_t data)
        {
            latest = data;
            total = data;
            count = 1;
        }

        void clearAverage(void) { total = count = 0; }
    };

    typedef QMap<ETcpMessage, CTcpInfo> TTcpMap;

    CSerialPort *serialPort;
    CTcpServer *tcpServer;
    TSerial2TcpMap serial2TcpMap;
    TTcpMap tcpDataMap;
    QTimer *sendTcpTimer;

    void initSerial2TcpMap(void);
    void initLua(void);
    void registerLuaDataFunc(const char *name, ETcpMessage msg,
                             const char *submod=NULL,
                             lua_CFunction func=luaGetGenericData);
    void registerLuaRobotModule(void);
    void runScript(const QByteArray &script);
    TLuaScriptMap getLuaScripts(void);
    void sendLuaScripts(void);

private slots:
    void handleSerialText(const QByteArray &text);
    void handleSerialMSG(ESerialMessage msg, const QByteArray &data);
    void clientConnected(void);
    void parseClientTcp(QDataStream &stream);
    void enableRP6Slave(void);
    void sendTcpData(void);
    
public:
    CControl(QObject *parent);

    // Lua bindings
    static int luaScriptRunning(lua_State *l);
    static int luaExecCmd(lua_State *l);
    static int luaSendText(lua_State *l);
    static int luaSendMsg(lua_State *l);
    static int luaUpdate(lua_State *l);
    static int luaGetTimeMS(lua_State *l);
    static int luaGetGenericData(lua_State *l);
    static int luaGetBumperLeft(lua_State *l);
    static int luaGetBumperRight(lua_State *l);
    static int luaGetACSLeft(lua_State *l);
    static int luaGetACSRight(lua_State *l);
    static int luaGetMoveComplete(lua_State *l);
    static int luaGetMotorDirLeft(lua_State *l);
    static int luaGetMotorDirRight(lua_State *l);
    static int luaGetACSPower(lua_State *l);
    static int luaGetRC5Key(lua_State *l);
    static int luaGetRC5Device(lua_State *l);
    static int luaGetRC5Toggle(lua_State *l);

    static void daemonMsgHandler(QtMsgType type, const char *msg);
};

#endif
