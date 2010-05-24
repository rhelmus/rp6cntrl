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

    enum EDataSerialType { DATA_BYTE, DATA_WORD };
    
    struct SSerial2TcpInfo
    {
        EDataSerialType dataType;
        const char *tcpVariable;
        SSerial2TcpInfo(void) {}
        SSerial2TcpInfo(EDataSerialType d,
                        const char *t) : dataType(d), tcpVariable(t) {}
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

    static int luaExecCmd(lua_State *l);
};

#endif
