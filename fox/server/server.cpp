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

    initSerial2TcpMap();
    initLua();
}

void CControl::initSerial2TcpMap()
{
    serial2TcpMap.insert(SERIAL_STATE_SENSORS, SSerial2TcpInfo(DATA_BYTE, "state"));

    serial2TcpMap.insert(SERIAL_BASE_LEDS, SSerial2TcpInfo(DATA_BYTE, "baseleds"));
    serial2TcpMap.insert(SERIAL_M32_LEDS, SSerial2TcpInfo(DATA_BYTE, "m32leds"));
    
    serial2TcpMap.insert(SERIAL_LIGHT_LEFT, SSerial2TcpInfo(DATA_WORD, "lightleft"));
    serial2TcpMap.insert(SERIAL_LIGHT_RIGHT, SSerial2TcpInfo(DATA_WORD, "lightright"));
    
    serial2TcpMap.insert(SERIAL_MOTOR_SPEED_LEFT, SSerial2TcpInfo(DATA_BYTE, "speedleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_SPEED_RIGHT, SSerial2TcpInfo(DATA_BYTE, "speedright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTSPEED_LEFT, SSerial2TcpInfo(DATA_BYTE, "destspeedleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTSPEED_RIGHT, SSerial2TcpInfo(DATA_BYTE, "destspeedright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIST_LEFT, SSerial2TcpInfo(DATA_WORD, "distleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIST_RIGHT, SSerial2TcpInfo(DATA_WORD, "distright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTDIST_LEFT, SSerial2TcpInfo(DATA_WORD, "destdistleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTDIST_RIGHT, SSerial2TcpInfo(DATA_WORD, "destdistright"));
    serial2TcpMap.insert(SERIAL_MOTOR_CURRENT_LEFT, SSerial2TcpInfo(DATA_WORD, "motorcurrentleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_CURRENT_RIGHT, SSerial2TcpInfo(DATA_WORD, "motorcurrentright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIRECTIONS, SSerial2TcpInfo(DATA_BYTE, "motordir"));

    serial2TcpMap.insert(SERIAL_BATTERY, SSerial2TcpInfo(DATA_WORD, "battery"));

    serial2TcpMap.insert(SERIAL_ACS_POWER, SSerial2TcpInfo(DATA_BYTE, "acspower"));

    serial2TcpMap.insert(SERIAL_MIC, SSerial2TcpInfo(DATA_WORD, "mic"));

    serial2TcpMap.insert(SERIAL_LASTRC5, SSerial2TcpInfo(DATA_WORD, "rc5"));
}

void CControl::initLua()
{
    luaInterface.registerFunction(luaExecCmd, "exec", this);

    lua_newtable(luaInterface);
    for (QMap<ESerialMessage, SSerial2TcpInfo>::iterator it=serial2TcpMap.begin();
         it!=serial2TcpMap.end(); ++it)
    {
        lua_pushinteger(luaInterface, 0);
        lua_setfield(luaInterface, -2, it.value().tcpVariable);
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
    TLuaScriptMap scripts = getLuaScripts();
    tcpServer->sendKeyValue("scripts", QVariant(scripts.keys()));
}

void CControl::handleSerialText(const QByteArray &text)
{
    if (text == "[READY]")
    {
        serialPort->sendCommand("s");
        QTimer::singleShot(4000, this, SLOT(enableRP6Slave()));
    }
    else
        tcpServer->sendText("rawserial", text);
}

void CControl::handleSerialMSG(ESerialMessage msg, const QByteArray &data)
{
    switch (serial2TcpMap[msg].dataType)
    {
        case DATA_BYTE:
            tcpServer->sendKeyValue(serial2TcpMap[msg].tcpVariable, data[0]);
            break;
        case DATA_WORD:
        {
            const uint16_t d = (uint8_t)data[0] + ((uint8_t)data[1] << 8);
            tcpServer->sendKeyValue(serial2TcpMap[msg].tcpVariable, d);
            break;
        }
    }    
}

void CControl::parseClientTcp(QDataStream &stream)
{
    QString msg;
    stream >> msg;

    if (msg == "command")
    {
        QString cmd;
        stream >> cmd;
        serialPort->sendCommand(cmd);
        qDebug() << "Received client cmd:" << cmd;
    }
    else if (msg == "getscripts")
        sendLuaScripts();
    else if (msg == "runlua")
    {
        QByteArray script;
        stream >> script;
        luaInterface.runScript(script.constData());
    }
    else if ((msg == "uploadlua") || (msg == "uprunlua"))
    {
        QString name;
        QByteArray script;
        stream >> name >> script;

        TLuaScriptMap scripts = getLuaScripts();
        scripts[name] = script;
        QSettings().setValue("scripts", scripts);

        sendLuaScripts();

        if (msg == "uprunlua")
            luaInterface.runScript(script.constData());
    }
    else if (msg == "runlocallua")
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        if (scripts.find(name) != scripts.end())
            luaInterface.runScript(scripts[name].toString().toLatin1().data());
    }
    else if (msg == "removelocallua")
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        scripts.remove(name);
        QSettings().setValue("scripts", scripts);
        
        sendLuaScripts();
    }
    else if (msg == "getlocallua")
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        if (scripts.find(name) != scripts.end())
            tcpServer->sendKeyValue("reqscript", scripts[name]);
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
