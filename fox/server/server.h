#ifndef CONTROL_H
#define CONTROL_H

#include <QMap>
#include <QObject>

#include "lua.h"
#include "shared.h"

class CSerialPort;
class CTcpServer;

class CControl: public QObject
{
    Q_OBJECT
  
    struct SSerial2TcpInfo
    {
        ETcpMessage tcpMessage;
        const char *luaKey;
        SSerial2TcpInfo(void) {}
        SSerial2TcpInfo(ETcpMessage m, const char *l) : tcpMessage(m), luaKey(l) {}
    };

    typedef QMap<QString, QVariant> TLuaScriptMap;
    
    CSerialPort *serialPort;
    CTcpServer *tcpServer;
    QMap<ESerialMessage, SSerial2TcpInfo> serial2TcpMap;
    CLuaInterface luaInterface;

    void initSerial2TcpMap(void);
    void initLua(void);
    TLuaScriptMap getLuaScripts(void);
    void sendLuaScripts(void);

private slots:
    void handleSerialText(const QByteArray &text);
    void handleSerialMSG(ESerialMessage msg, const QByteArray &data);
    void parseClientTcp(QDataStream &stream);
    void enableRP6Slave(void);
    
public:
    CControl(QObject *parent);

    // Lua bindings
    static int luaExecCmd(lua_State *l);
    static int luaSendText(lua_State *l);
};

#endif
