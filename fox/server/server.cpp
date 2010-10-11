#include <assert.h>
#include <sys/time.h>

#include <QtCore>

#include <luanav.h>
#include "pathengine.h"
#include "serial.h"
#include "server.h"
#include "shared.h"
#include "tcp.h"

namespace {

const char *moveDirToLuaStr(int dir)
{
    switch (dir)
    {
    case FWD: return "fwd";
    case BWD: return "bwd";
    case LEFT: return "left";
    case RIGHT: return "right";
    }

    assert(false);
    return NULL;
}

}

CControl::CControl(QObject *parent) : QObject(parent)
{
    QStringList args(QCoreApplication::arguments());
    QString port = "/dev/ttyUSB0";
    QString preva;
    bool daemonize = false;

    foreach(QString a, args)
    {
        if (preva == "-d")
            port = a;
        else if (a == "-D")
            daemonize = true;
        preva = a;
    }

    if (daemonize)
    {
        qInstallMsgHandler(daemonMsgHandler);
        if (daemon(1, 0) == -1) // Keep working dir, close standard fd's
            qFatal("Failed to daemonize!");
    }

    serialPort = new CSerialPort(this, port);
    connect(serialPort, SIGNAL(textAvailable(const QByteArray &)), this,
            SLOT(handleSerialText(const QByteArray &)));
    connect(serialPort, SIGNAL(msgAvailable(ESerialMessage, const QByteArray &)),
            this, SLOT(handleSerialMSG(ESerialMessage, const QByteArray &)));

    tcpServer = new CTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(clientConnected()));
    connect(tcpServer, SIGNAL(clientTcpReceived(QDataStream &)), this,
            SLOT(parseClientTcp(QDataStream &)));

    // important: Keep these 3 in order
    initTcpDataTypes();
    initSerial2TcpMap();
    initLua();

    sendTcpTimer = new QTimer(this);
    connect(sendTcpTimer, SIGNAL(timeout()), this, SLOT(sendTcpData()));
    sendTcpTimer->start(500);
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
    NLuaNav::registerBindings();

    NLua::registerFunction(luaScriptRunning, "scriptrunning", this);
    NLua::registerFunction(luaExecCmd, "exec", this);
    NLua::registerFunction(luaSendText, "sendtext", this);
    NLua::registerFunction(luaSendMsg, "sendmsg", this);
    NLua::registerFunction(luaUpdate, "update");
    NLua::registerFunction(luaGetTimeMS, "gettimems");

    registerLuaRobotModule();

    NLua::luaInterface.exec();
}

void CControl::registerLuaDataFunc(const char *name, ETcpMessage msg,
                                   const char *submod, lua_CFunction func)
{
    lua_getglobal(NLua::luaInterface, "robot");

    if (lua_isnil(NLua::luaInterface, -1))
    {
        lua_pop(NLua::luaInterface, 1);
        lua_newtable(NLua::luaInterface);
    }

    if (submod)
    {
        lua_getfield(NLua::luaInterface, -1, submod);

        if (lua_isnil(NLua::luaInterface, -1))
        {
            lua_pop(NLua::luaInterface, 1);
            lua_newtable(NLua::luaInterface);
        }
    }

    lua_pushlightuserdata(NLua::luaInterface, this);
    lua_pushinteger(NLua::luaInterface, msg);
    lua_pushcclosure(NLua::luaInterface, func, 2);
    lua_setfield(NLua::luaInterface, -2, name);

    if (submod)
        lua_setfield(NLua::luaInterface, -2, submod);

    lua_setglobal(NLua::luaInterface, "robot");
}

void CControl::registerLuaRobotModule()
{
    registerLuaDataFunc("bumperleft", TCP_STATE_SENSORS, "sensors", luaGetBumperLeft);
    registerLuaDataFunc("bumperright", TCP_STATE_SENSORS, "sensors", luaGetBumperRight);
    registerLuaDataFunc("acsleft", TCP_STATE_SENSORS, "sensors", luaGetACSLeft);
    registerLuaDataFunc("acsright", TCP_STATE_SENSORS, "sensors", luaGetACSRight);
    registerLuaDataFunc("movecomplete", TCP_STATE_SENSORS, "motor", luaGetMoveComplete);
    registerLuaDataFunc("base", TCP_BASE_LEDS, "leds");
    registerLuaDataFunc("m32", TCP_M32_LEDS, "leds");
    registerLuaDataFunc("lightleft", TCP_LIGHT_LEFT, "sensors");
    registerLuaDataFunc("lightright", TCP_LIGHT_RIGHT, "sensors");
    registerLuaDataFunc("speedleft", TCP_MOTOR_SPEED_LEFT, "motor");
    registerLuaDataFunc("speedright", TCP_MOTOR_SPEED_RIGHT, "motor");
    registerLuaDataFunc("destspeedleft", TCP_MOTOR_DESTSPEED_LEFT, "motor");
    registerLuaDataFunc("destspeedright", TCP_MOTOR_DESTSPEED_RIGHT, "motor");
    registerLuaDataFunc("distleft", TCP_MOTOR_DIST_LEFT, "motor");
    registerLuaDataFunc("distright", TCP_MOTOR_DIST_RIGHT, "motor");
    registerLuaDataFunc("destdistleft", TCP_MOTOR_DESTDIST_LEFT, "motor");
    registerLuaDataFunc("destdistright", TCP_MOTOR_DESTDIST_RIGHT, "motor");
    registerLuaDataFunc("motorcurrentleft", TCP_MOTOR_CURRENT_LEFT, "motor");
    registerLuaDataFunc("motorcurrentright", TCP_MOTOR_CURRENT_RIGHT, "motor");
    registerLuaDataFunc("motordirleft", TCP_MOTOR_DIRECTIONS, "motor", luaGetMotorDirLeft);
    registerLuaDataFunc("motordirright", TCP_MOTOR_DIRECTIONS, "motor", luaGetMotorDirRight);
    registerLuaDataFunc("battery", TCP_BATTERY, "sensors");
    registerLuaDataFunc("acspower", TCP_ACS_POWER, "sensors", luaGetACSPower);
    registerLuaDataFunc("mic", TCP_MIC, "sensors");
    registerLuaDataFunc("key", TCP_LASTRC5, "rc5", luaGetRC5Key);
    registerLuaDataFunc("device", TCP_LASTRC5, "rc5", luaGetRC5Device);
    registerLuaDataFunc("toggle", TCP_LASTRC5, "rc5", luaGetRC5Toggle);
    registerLuaDataFunc("sharpir", TCP_SHARPIR, "sensors");
}

void CControl::runScript(const QByteArray &script)
{
    NLua::runScript(script);
    if (tcpServer->hasConnections())
        NLua::scriptInitClient();
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
    const ETcpMessage tcpmsg = serial2TcpMap[msg].tcpMessage;
    int tcpdata;

    switch (tcpDataTypes[tcpmsg])
    {
        case DATA_BYTE:
            tcpdata = static_cast<uint8_t>(data[0]);
            break;
        case DATA_WORD:
        {
            const uint16_t d = static_cast<uint8_t>(data[0]) +
                (static_cast<uint8_t>(data[1]) << 8);
            tcpdata = d;
            break;
        }
    }

    // Store data and sum if we want it averaged
    switch (tcpmsg)
    {
    case TCP_STATE_SENSORS:
    case TCP_BASE_LEDS:
    case TCP_M32_LEDS:
    case TCP_MOTOR_DESTSPEED_LEFT:
    case TCP_MOTOR_DESTSPEED_RIGHT:
    case TCP_MOTOR_DIST_LEFT:
    case TCP_MOTOR_DIST_RIGHT:
    case TCP_MOTOR_DESTDIST_LEFT:
    case TCP_MOTOR_DESTDIST_RIGHT:
    case TCP_MOTOR_DIRECTIONS:
    case TCP_ACS_POWER:
    case TCP_LASTRC5:
        tcpDataMap[tcpmsg].setData(tcpdata);
        break;
    case TCP_LIGHT_LEFT:
    case TCP_LIGHT_RIGHT:
    case TCP_MOTOR_SPEED_LEFT:
    case TCP_MOTOR_SPEED_RIGHT:
    case TCP_MOTOR_CURRENT_LEFT:
    case TCP_MOTOR_CURRENT_RIGHT:
    case TCP_BATTERY:
    case TCP_MIC:
    case TCP_SHARPIR:
        tcpDataMap[tcpmsg].addData(tcpdata);
        break;
    default:
        break;
    }
}

void CControl::clientConnected()
{
    NLua::scriptInitClient();
}

void CControl::parseClientTcp(QDataStream &stream)
{
    uint8_t m;
    stream >> m;
    ETcpMessage msg = static_cast<ETcpMessage>(m);

    qDebug() << "msg: " << m;
    
    if (msg == TCP_UPDATEDELAY)
    {
        uint16_t delay;
        stream >> delay;
        sendTcpTimer->setInterval(delay);
    }
    else if (msg == TCP_COMMAND)
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
        runScript(script.constData());
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
            runScript(script);
    }
    else if (msg == TCP_RUNSERVERLUA)
    {
        QString name;
        stream >> name;
        
        TLuaScriptMap scripts = getLuaScripts();
        if (scripts.find(name) != scripts.end())
            runScript(scripts[name].toByteArray());
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
    else if (msg == TCP_LUACOMMAND)
    {
        QString cmd;
        QStringList args;
        stream >> cmd >> args;
        NLua::execScriptCmd(cmd, args);
    }
}

void CControl::enableRP6Slave()
{
    serialPort->sendCommand("set power 1");
    serialPort->sendCommand("set slave 1");
}

void CControl::sendTcpData()
{
    for (TTcpMap::iterator it=tcpDataMap.begin(); it!=tcpDataMap.end(); ++it)
    {
        switch (tcpDataTypes[it.key()])
        {
            case DATA_BYTE:
                tcpServer->send(it.key(), static_cast<uint8_t>(it.value().data()));
                break;
            case DATA_WORD:
            {
                tcpServer->send(it.key(), static_cast<uint16_t>(it.value().data()));
                break;
            }
        }

        it.value().clearAverage();
    }
}

int CControl::luaScriptRunning(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    control->tcpServer->send(TCP_SCRIPTRUNNING, static_cast<bool>(lua_toboolean(l, 1)));
    return 0;
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

int CControl::luaSendMsg(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    const char *msg = luaL_checkstring(l, 1);
    const int nargs = lua_gettop(l);

    CTcpMsgComposer comp(TCP_LUAMSG);

    comp << QString(msg);

    for (int i=2; i<=nargs; ++i)
    {
        switch (lua_type(l, i))
        {
        case LUA_TBOOLEAN: comp << static_cast<bool>(lua_toboolean(l, i)); break;
        case LUA_TNUMBER: comp << static_cast<float>(lua_tonumber(l, i)); break;
        default:
            qWarning() << "Unknown type: " << lua_typename(l, i) <<
                    ". Converting to string...";
            // Fall through
        case LUA_TSTRING: comp << QString(lua_tostring(l, i)); break;
        case LUA_TTABLE: comp << NLua::convertLuaTable(l, i); break;
        }
    }

    control->tcpServer->send(comp);

    return 0;
}


int CControl::luaUpdate(lua_State *l)
{
    int timeout = luaL_checkint(l, 1);
    QCoreApplication::processEvents(QEventLoop::AllEvents, timeout);
    return 0;
}

int CControl::luaGetTimeMS(lua_State *l)
{
    static timeval start, current;
    static bool started = false;

    if (!started)
    {
        gettimeofday(&start, NULL);
        started = true;
    }

    gettimeofday(&current, NULL);
    lua_pushinteger(l, ((current.tv_sec-start.tv_sec) * 1000 ) +
                    ((current.tv_usec-start.tv_usec) / 1000));

    return 1;
}

int CControl::luaGetGenericData(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    lua_pushinteger(l, control->tcpDataMap[msg].latestData());
    return 1;
}

int CControl::luaGetBumperLeft(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SStateSensors state;
    state.byte = control->tcpDataMap[msg].latestData();
    lua_pushboolean(l, state.bumperLeft);
    return 1;
}

int CControl::luaGetBumperRight(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SStateSensors state;
    state.byte = control->tcpDataMap[msg].latestData();
    lua_pushboolean(l, state.bumperRight);
    return 1;
}

int CControl::luaGetACSLeft(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SStateSensors state;
    state.byte = control->tcpDataMap[msg].latestData();
    lua_pushboolean(l, state.ACSLeft);
    return 1;
}

int CControl::luaGetACSRight(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SStateSensors state;
    state.byte = control->tcpDataMap[msg].latestData();
    lua_pushboolean(l, state.ACSRight);
    return 1;
}

int CControl::luaGetMoveComplete(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SStateSensors state;
    state.byte = control->tcpDataMap[msg].latestData();
    lua_pushboolean(l, state.movementComplete);
    return 1;
}

int CControl::luaGetMotorDirLeft(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SMotorDirections dir;
    dir.byte = control->tcpDataMap[msg].latestData();
    lua_pushstring(l, moveDirToLuaStr(dir.left));
    return 1;
}

int CControl::luaGetMotorDirRight(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    SMotorDirections dir;
    dir.byte = control->tcpDataMap[msg].latestData();
    lua_pushstring(l, moveDirToLuaStr(dir.right));
    return 1;
}

int CControl::luaGetACSPower(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    EACSPowerState state = static_cast<EACSPowerState>(control->tcpDataMap[msg].latestData());

    switch (state)
    {
    case ACS_POWER_OFF: lua_pushstring(l, "off"); break;
    case ACS_POWER_LOW: lua_pushstring(l, "low"); break;
    case ACS_POWER_MED: lua_pushstring(l, "med"); break;
    case ACS_POWER_HIGH: lua_pushstring(l, "high"); break;
    }

    return 1;
}

int CControl::luaGetRC5Key(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    RC5data_t rc5;
    rc5.data = control->tcpDataMap[msg].latestData();
    lua_pushnumber(l, rc5.key_code);
    return 1;
}

int CControl::luaGetRC5Device(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    RC5data_t rc5;
    rc5.data = control->tcpDataMap[msg].latestData();
    lua_pushnumber(l, rc5.device);
    return 1;
}

int CControl::luaGetRC5Toggle(lua_State *l)
{
    CControl *control = static_cast<CControl *>(lua_touserdata(l, lua_upvalueindex(1)));
    ETcpMessage msg = static_cast<ETcpMessage>(lua_tointeger(l, lua_upvalueindex(2)));
    RC5data_t rc5;
    rc5.data = control->tcpDataMap[msg].latestData();
    lua_pushboolean(l, rc5.toggle_bit);
    return 1;
}

void CControl::daemonMsgHandler(QtMsgType type, const char *msg)
{
    QFile logfile("server.log");
    QFile::OpenMode om = QFile::WriteOnly;

    if (logfile.size() > (1024 * 1024 * 1024))
        om |= QFile::Truncate;
    else
        om |= QFile::Append;

    if (logfile.open(om))
    {
        QTextStream stream(&logfile);

        stream << QDateTime::currentDateTime().toString() << " - ";

        switch (type)
        {
        case QtDebugMsg: stream << "[Debug]: "; break;
        case QtWarningMsg: stream << "[Warning]: "; break;
        case QtCriticalMsg: stream << "[Critical]: "; break;
        case QtFatalMsg: stream << "[Fatal]: "; break;
        }

        stream << " " << msg << "\n";

        if (type == QtFatalMsg)
            abort();
    }
}
