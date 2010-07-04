#include <assert.h>

#include <QtCore>

#include "serial.h"
#include "server.h"
#include "shared.h"
#include "tcp.h"

CControl::CControl(QObject *parent) : QObject(parent)
{
    QStringList args(QCoreApplication::arguments());
    QString port = (args.size() > 1) ? args.at(1) : "/dev/ttyUSB0";
    
    serialPort = new CSerialPort(this, port);
    connect(serialPort, SIGNAL(textAvailable(const QByteArray &)), this,
            SLOT(handleSerialText(const QByteArray &)));
    connect(serialPort, SIGNAL(msgAvailable(ESerialMessage, const QByteArray &)),
            this, SLOT(handleSerialMSG(ESerialMessage, const QByteArray &)));

    tcpServer = new CTcpServer(this);
    connect(tcpServer, SIGNAL(clientTcpReceived(QDataStream &)), this,
            SLOT(parseClientTcp(QDataStream &)));

    initTcpDataTypes();
    initSerial2TcpMap();
    initLua();
}

void CControl::initSerial2TcpMap()
{
    serial2TcpMap.insert(SERIAL_STATE_SENSORS,
                         SSerial2TcpInfo(TCP_STATE_SENSORS, "state"));

    serial2TcpMap.insert(SERIAL_BASE_LEDS,
                         SSerial2TcpInfo(TCP_BASE_LEDS, "baseleds"));
    serial2TcpMap.insert(SERIAL_M32_LEDS,
                         SSerial2TcpInfo(TCP_M32_LEDS, "m32leds"));
    
    serial2TcpMap.insert(SERIAL_LIGHT_LEFT,
                         SSerial2TcpInfo(TCP_LIGHT_LEFT, "lightleft"));
    serial2TcpMap.insert(SERIAL_LIGHT_RIGHT,
                         SSerial2TcpInfo(TCP_LIGHT_RIGHT, "lightright"));
    
    serial2TcpMap.insert(SERIAL_MOTOR_SPEED_LEFT,
                         SSerial2TcpInfo(TCP_MOTOR_SPEED_LEFT, "speedleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_SPEED_RIGHT,
                         SSerial2TcpInfo(TCP_MOTOR_SPEED_RIGHT, "speedright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTSPEED_LEFT,
                         SSerial2TcpInfo(TCP_MOTOR_DESTSPEED_LEFT, "destspeedleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTSPEED_RIGHT,
                         SSerial2TcpInfo(TCP_MOTOR_DESTSPEED_RIGHT, "destspeedright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIST_LEFT,
                         SSerial2TcpInfo(TCP_MOTOR_DIST_LEFT, "distleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIST_RIGHT,
                         SSerial2TcpInfo(TCP_MOTOR_DIST_RIGHT, "distright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTDIST_LEFT,
                         SSerial2TcpInfo(TCP_MOTOR_DESTDIST_LEFT, "destdistleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTDIST_RIGHT,
                         SSerial2TcpInfo(TCP_MOTOR_DESTDIST_RIGHT, "destdistright"));
    serial2TcpMap.insert(SERIAL_MOTOR_CURRENT_LEFT,
                         SSerial2TcpInfo(TCP_MOTOR_CURRENT_LEFT, "motorcurrentleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_CURRENT_RIGHT,
                         SSerial2TcpInfo(TCP_MOTOR_CURRENT_RIGHT, "motorcurrentright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIRECTIONS,
                         SSerial2TcpInfo(TCP_MOTOR_DIRECTIONS, "motordirections"));

    serial2TcpMap.insert(SERIAL_BATTERY, SSerial2TcpInfo(TCP_BATTERY, "battery"));

    serial2TcpMap.insert(SERIAL_ACS_POWER, SSerial2TcpInfo(TCP_ACS_POWER, "acspower"));

    serial2TcpMap.insert(SERIAL_MIC, SSerial2TcpInfo(TCP_MIC, "mic"));

    serial2TcpMap.insert(SERIAL_LASTRC5, SSerial2TcpInfo(TCP_LASTRC5, "rc5"));

    serial2TcpMap.insert(SERIAL_SHARPIR, SSerial2TcpInfo(TCP_SHARPIR, "sharpir"));
}

void CControl::initLua()
{
    luaInterface.registerFunction(luaExecCmd, "exec", this);
    luaInterface.registerFunction(luaSendText, "sendtext", this);
    luaInterface.registerFunction(luaUpdate, "update");

    lua_newtable(luaInterface);
    for (QMap<ESerialMessage, SSerial2TcpInfo>::iterator it=serial2TcpMap.begin();
         it!=serial2TcpMap.end(); ++it)
    {
        lua_pushinteger(luaInterface, 0);
        lua_setfield(luaInterface, -2, it.value().luaKey);
    }
    lua_setglobal(luaInterface, "sensortable");
    
    luaInterface.exec();
}

CControl::TLuaScriptMap CControl::getLuaScripts()
{
    QSettings settings;
    return settings.value("scripts").toMap();
}

void CControl::sendLuaScripts()
{
    tcpServer->send(TCP_LUASCRIPTS, getLuaScripts().keys());
}

void CControl::handleSerialText(const QByteArray &text)
{
    if (text == "[READY]")
    {
        serialPort->sendCommand("s");
        QTimer::singleShot(4000, this, SLOT(enableRP6Slave()));
    }
    else
        tcpServer->send(TCP_RAWSERIAL, text);
}

void CControl::handleSerialMSG(ESerialMessage msg, const QByteArray &data)
{
/*    qDebug() << "Serial data: " << msg << " type: " <<
        tcpDataTypes[serial2TcpMap[msg].tcpMessage] << " name: " <<
        serial2TcpMap[msg].luaKey << " TCP type: " <<
        serial2TcpMap[msg].tcpMessage;*/
        
    switch (tcpDataTypes[serial2TcpMap[msg].tcpMessage])
    {
        case DATA_BYTE:
            tcpServer->send(serial2TcpMap[msg].tcpMessage,
                            static_cast<uint8_t>(data[0]));
            break;
        case DATA_WORD:
        {
            const uint16_t d = static_cast<uint8_t>(data[0]) +
                (static_cast<uint8_t>(data[1]) << 8);
            tcpServer->send(serial2TcpMap[msg].tcpMessage, d);
            break;
        }
    }    
}

void CControl::parseClientTcp(QDataStream &stream)
{
    uint8_t m;
    stream >> m;
    ETcpMessage msg = static_cast<ETcpMessage>(m);

    qDebug() << "msg: " << m;
    
    if (msg == TCP_COMMAND)
    {
        QString cmd;
        stream >> cmd;
        serialPort->sendCommand(cmd);
        qDebug() << "Received client cmd:" << cmd;
    }
    else if (msg == TCP_GETSCRIPTS)
        sendLuaScripts();
    else if (msg == TCP_RUNLUA)
    {
        QByteArray script;
        stream >> script;
        qDebug() << "Received script:\n" << script;
        luaInterface.runScript(script.constData());
    }
    else if ((msg == TCP_UPLOADLUA) || (msg == TCP_UPRUNLUA))
    {
        QString name;
        QByteArray script;
        stream >> name >> script;

        TLuaScriptMap scripts = getLuaScripts();
        scripts[name] = script;
        QSettings().setValue("scripts", scripts);

        sendLuaScripts();

        if (msg == TCP_UPRUNLUA)
            luaInterface.runScript(script.constData());
    }
    else if (msg == TCP_RUNSERVERLUA)
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        if (scripts.find(name) != scripts.end())
            luaInterface.runScript(scripts[name].toString().toLatin1().data());
    }
    else if (msg == TCP_REMOVESERVERLUA)
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        scripts.remove(name);
        QSettings().setValue("scripts", scripts);
        
        sendLuaScripts();
    }
    else if (msg == TCP_GETSERVERLUA)
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        if (scripts.find(name) != scripts.end())
            tcpServer->send(TCP_REQUESTEDSCRIPT, scripts[name].toByteArray());
    }
}

void CControl::enableRP6Slave()
{
    serialPort->sendCommand("set power 1");
    serialPort->sendCommand("set slave 1");
}

int CControl::luaExecCmd(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    const char *cmd = luaL_checkstring(l, 1);
    qDebug() << "Exec cmd: " << cmd << "\n";
    control->serialPort->sendCommand(cmd);
    return 0;
}

int CControl::luaSendText(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    const char *txt = luaL_checkstring(l, 1);
    control->tcpServer->send(TCP_LUATEXT, QString(txt));
    return 0;
}

int CControl::luaUpdate(lua_State *)
{
    QCoreApplication::processEvents();
    return 0;
}
