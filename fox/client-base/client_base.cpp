#include <QHostAddress>
#include <QStringList>

#include "client_base.h"
#include "tcputil.h"

namespace {

QString directionString(uint8_t dir)
{
    switch (dir)
    {
        case FWD: return "forward";
        case BWD: return "backward";
        case LEFT: return "left";
        case RIGHT: return "right";
    }

    return QString();
}

}

CBaseClientTcpHandler::CBaseClientTcpHandler(CBaseClient *b) : baseClient(b),
                                                               tcpReadBlockSize(0)
{
    clientSocket = new QTcpSocket(this);
    connect(clientSocket, SIGNAL(connected()), this, SLOT(connectedToServer()));
    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(disconnectedFromServer()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));

    initTcpDataTypes();
}

void CBaseClientTcpHandler::connectedToServer(void)
{
    clientSocket->write(CTcpMsgComposer(TCP_GETSCRIPTS));
    
    baseClient->updateConnection(true);
    baseClient->appendLogOutput(QString("Connected to server (%1:%2)\n").
        arg(clientSocket->localAddress().toString()).
        arg(clientSocket->localPort()));
}

void CBaseClientTcpHandler::disconnectedFromServer(void)
{
    baseClient->updateConnection(false);
    baseClient->appendLogOutput("Disconnected from server.\n");
}

void CBaseClientTcpHandler::serverHasData()
{
    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_4_4);
    
    while (true)
    {
        if (tcpReadBlockSize == 0)
        {
            if (clientSocket->bytesAvailable() < (int)sizeof(quint32))
                return;
            
            in >> tcpReadBlockSize;
        }
        
        quint32 bytesbefore = clientSocket->bytesAvailable();
        if (bytesbefore < tcpReadBlockSize)
            return;

        baseClient->parseTcp(in);

        // Discard any extra unread data
        quint32 bytesafter = clientSocket->bytesAvailable();
        if (bytesafter > (bytesbefore - tcpReadBlockSize))
        {
            quint32 bytesremaining = bytesafter - (bytesbefore - tcpReadBlockSize);
            baseClient->appendLogOutput(QString("Warning: %1 bytes remaining").
                arg(bytesremaining));
            clientSocket->read(bytesremaining);
        }
        
        tcpReadBlockSize = 0;
    }
}

void CBaseClientTcpHandler::socketError(QAbstractSocket::SocketError)
{
    baseClient->tcpError(clientSocket->errorString());
    baseClient->updateConnection(false);
    baseClient->appendLogOutput(QString("Socket error: %1\n").
        arg(clientSocket->errorString()));
}

void CBaseClientTcpHandler::connectToHost(const QString &host)
{
    clientSocket->abort(); // Always disconnect first
    clientSocket->connectToHost(host, 40000);
}


CBaseClient::CBaseClient() : tcpHandler(this), driveForwardSpeed(0),
                             driveTurnSpeed(0), driveTurning(false)
{
    currentStateSensors.byte = 0;
    currentMotorDirections.byte = 0;
}

void CBaseClient::parseTcp(QDataStream &stream)
{
    uint8_t m;
    stream >> m;
    ETcpMessage msg = static_cast<ETcpMessage>(m);

    if (msg == TCP_RAWSERIAL)
    {
        QByteArray text;
        stream >> text;
        appendConsoleOutput(text + "\n");
    }
    // Robot data?
    else if ((msg > TCP_MIN_ROBOT_INDEX) && (msg < TCP_MAX_ROBOT_INDEX))
    {
        int data;
        if (tcpDataTypes[msg] == DATA_BYTE)
        {
            uint8_t b;
            stream >> b;
            data = b;
        }
        else // DATA_WORD
        {
            uint16_t w;
            stream >> w;
            data = w;
        }

        if (msg == TCP_STATE_SENSORS)
        {
            SStateSensors state;
            state.byte = data;
            
            tcpRobotStateUpdate(currentStateSensors, state);

            if (currentStateSensors.movementComplete && !state.movementComplete)
                appendLogOutput("Started movement.");
            else if (!currentStateSensors.movementComplete && state.movementComplete)
                appendLogOutput("Finished movement.");
            
            if (msg == TCP_STATE_SENSORS)
                currentStateSensors = state;
        }
        else
        {
            if (msg == TCP_MOTOR_DIRECTIONS)
            {
                SMotorDirections dir;
                dir.byte = data;
                updateMotorDirections(dir);
            }
            
            tcpHandleRobotData(msg, data);
        }
    }
    else if (msg == TCP_LUASCRIPTS)
    {
        QStringList scripts;
        stream >> scripts;
        tcpLuaScripts(scripts);
    }
    else if (msg == TCP_REQUESTEDSCRIPT)
    {
        QByteArray script;
        stream >> script;
        tcpRequestedScript(script);
    }
    else if (msg == TCP_SCRIPTRUNNING)
    {
        bool r;
        stream >> r;
        tcpScriptRunning(r);
        qDebug() << "script running:" << r;
    }
    else if (msg == TCP_LUATEXT)
    {
        QString text;
        stream >> text;
        appendLuaOutput(text);
    }
    else if (msg == TCP_LUAMSG)
    {
        QString msg;
        stream >> msg;
        tcpHandleLuaMsg(msg, stream);
    }
}

void CBaseClient::updateMotorDirections(const SMotorDirections &dir)
{
    if (dir.byte != currentMotorDirections.byte)
    {
        QString leftS(directionString(dir.left)), rightS(directionString(dir.right));
        if (dir.left != currentMotorDirections.left)
            appendLogOutput(QString("Changed left motor direction to %1.\n").arg(leftS));
        
        if (dir.right != currentMotorDirections.right)
            appendLogOutput(QString("Changed right motor direction to %1.\n").arg(rightS));
        
        if (dir.destLeft != currentMotorDirections.destLeft)
            appendLogOutput(QString("Changed left destination motor direction to %1.\n").arg(directionString(dir.destLeft)));
        
        if (dir.destRight != currentMotorDirections.destRight)
            appendLogOutput(QString("Changed right destination motor direction to %1.\n").arg(directionString(dir.destRight)));
        
        currentMotorDirections.byte = dir.byte;
    }
}

void CBaseClient::changeDriveSpeedVar(int &speed, int delta)
{
    const int minflipspeed = 30, maxspeed = 160;
    
    speed += delta;
    
    if (delta > 0)
    {
        if ((speed < 0) && (-(speed) < minflipspeed))
            speed = -(speed);
        else if ((speed > 0) && (speed < minflipspeed))
            speed = minflipspeed;
    }
    else
    {
        if ((speed > 0) && (speed < minflipspeed))
            speed = -(speed);
        else if ((speed < 0) && (-(speed) < minflipspeed))
            speed = -minflipspeed;
    }
    
    if (speed < -maxspeed)
        speed = -maxspeed;
    else if (speed > maxspeed)
        speed = maxspeed;
}

void CBaseClient::executeCommand(const QString &cmd)
{
    appendLogOutput(QString("Executing RP6 console command: \"%1\"\n").arg(cmd));
    
    if (connected())
        tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_COMMAND) << cmd);
    else
        appendLogOutput("WARNING: Cannot execute command: not connected!\n");
}


void CBaseClient::updateDriving(int dir)
{
    const int speedchange = 5, startspeed = 60;

    if (currentStateSensors.bumperLeft || currentStateSensors.bumperRight ||
        !currentStateSensors.movementComplete)
        return;
    
    if (driveForwardSpeed == 0)
        driveForwardSpeed = (dir == BWD) ? -startspeed : startspeed;
    
    // Change from left/right to up/down?   
    if (((dir == FWD) || (dir == BWD)) && driveTurning)
        driveTurnSpeed = 0;
    
    if (dir == FWD)
        changeDriveSpeedVar(driveForwardSpeed, speedchange);
    else if (dir == BWD)
        changeDriveSpeedVar(driveForwardSpeed, -speedchange);
    else if (dir == RIGHT)
        changeDriveSpeedVar(driveTurnSpeed, speedchange);
    else if (dir == LEFT)
        changeDriveSpeedVar(driveTurnSpeed, -speedchange);
    
    driveTurning = ((dir == LEFT) || (dir == RIGHT));

    if (driveForwardSpeed < 0)
    {
        if ((currentMotorDirections.destLeft != BWD) || (currentMotorDirections.destRight != BWD))
            executeCommand("set dir bwd");
    }
    else
    {
        if ((currentMotorDirections.destLeft != FWD) || (currentMotorDirections.destRight != FWD))
            executeCommand("set dir fwd");
    }
    
    if (driveTurning)
    {
        const int speed = abs(driveForwardSpeed);
        int left, right;
        
        if (driveTurnSpeed < 0) // Left
        {
            left = speed - ((speed < -driveTurnSpeed) ? speed : -driveTurnSpeed);
            right = speed;
        }
        else // right
        {
            left = speed;
            right = speed - ((speed < driveTurnSpeed) ? speed : driveTurnSpeed);
        }
        
        executeCommand(QString("set speed %1 %2").arg(left).arg(right));
        
        updateDriveSpeed((driveForwardSpeed < 0) ? -left : left,
                         (driveForwardSpeed < 0) ? -right : right);
    }
    else
    {
        executeCommand(QString("set speed %1 %1").arg(abs(driveForwardSpeed)));
        updateDriveSpeed(driveForwardSpeed, driveForwardSpeed);
    }
}

void CBaseClient::stopDrive()
{
    executeCommand("stop");
    driveForwardSpeed = 0;
    driveTurnSpeed = 0;
    driveTurning = false;
    updateDriveSpeed(0, 0);
}

void CBaseClient::uploadLocalScript(const QString &name, const QByteArray &text)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_UPLOADLUA) << name << text);
}

void CBaseClient::runLocalScript(const QByteArray &text)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_RUNLUA) << text);
}

void CBaseClient::uploadRunLocalScript(const QString &name, const QByteArray &text)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_UPRUNLUA) << name << text);
}

void CBaseClient::runServerScript(const QString &name)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_RUNSERVERLUA) << name);
}

void CBaseClient::removeServerScript(const QString &name)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_REMOVESERVERLUA) << name);
}

void CBaseClient::downloadServerScript(const QString &name)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_GETSERVERLUA) << name);
}

void CBaseClient::executeScriptCommand(const QString &cmd, const QStringList &args)
{
    tcpHandler.getSocket()->write(CTcpMsgComposer(TCP_LUACOMMAND) << cmd << args);
}
