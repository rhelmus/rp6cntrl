#include <QtCore>
#include <QtNetwork>

#include "tui/button.h"
#include "tui/label.h"
#include "tui/textfield.h"
#include "tui/tui.h"
#include "client.h"
#include "display_widget.h"

CNCursManager::CNCursManager(QObject *parent) : QObject(parent)                                                
{
    // Setup ncurses
    NNCurses::TUI.InitNCurses();

    CNCursClient *w = new CNCursClient;
    w->SetFColors(COLOR_GREEN, COLOR_BLUE);
    w->SetDFColors(COLOR_WHITE, COLOR_BLUE);
    w->SetMinWidth(60);
    w->SetMinHeight(15);
    NNCurses::TUI.AddGroup(w, true);

    // Updates ncurses TUI
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNCurs()));
    timer->start(0);

    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this,
            SLOT(stopNCurs()));
}

void CNCursManager::updateNCurs()
{
    if (!NNCurses::TUI.Run())
        QCoreApplication::quit();
}

void CNCursManager::stopNCurs()
{
    NNCurses::TUI.StopNCurses();
}


CNCursClient::CNCursClient() : tcpReadBlockSize(0), activeScreen(NULL),
                               dataDisplays(DISPLAY_MAX_INDEX)
{
    StartPack(mainScreen = createMainScreen(), false, true, 0, 0);
    StartPack(logScreen = createLogScreen(), false, true, 0, 0);

    screenList.push_back(mainScreen);
    screenList.push_back(logScreen);

    enableScreen(mainScreen);

    appendLogText("Started NCurses client.\n");
}

CNCursClient::TScreen *CNCursClient::createMainScreen()
{
    TScreen *ret = new TScreen(CBox::VERTICAL, false);
    ret->Enable(false);

    NNCurses::CBox *hbox = new NNCurses::CBox(CBox::HORIZONTAL, true);
    ret->AddWidget(hbox);
    
    hbox->AddWidget(createMovementDisplay());
    hbox->AddWidget(createSensorDisplay());
    hbox->AddWidget(createOtherDisplay());
    
    ret->EndPack(connectButton = new NNCurses::CButton("Connect"), false, true, 0, 0);
    
    clientSocket = new QTcpSocket(this);
    connect(clientSocket, SIGNAL(connected()), this, SLOT(connectedToServer()));
    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(disconnectedFromServer()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    
    return ret;
}

CNCursClient::TScreen *CNCursClient::createLogScreen()
{
    TScreen *ret = new TScreen(CBox::HORIZONTAL, true);
    ret->Enable(false);

    ret->AddWidget(logWidget = new NNCurses::CTextField(30, 10, true));
    
    return ret;
}

void CNCursClient::enableScreen(TScreen *screen)
{
    if (activeScreen == screen)
        return;
    
    if (activeScreen)
        activeScreen->Enable(false);

    screen->Enable(true);
    activeScreen = screen;
}

void CNCursClient::appendLogText(std::string text)
{
    text = QTime::currentTime().toString().toStdString() + ": " + text;
    logWidget->AddText(text);
}

CDisplayWidget *CNCursClient::createMovementDisplay()
{
    CDisplayWidget *ret = new CDisplayWidget("Movement");

    dataDisplays[DISPLAY_SPEED] = ret->addDisplay("Speed");
    dataDisplays[DISPLAY_DISTANCE] = ret->addDisplay("Distance");
    dataDisplays[DISPLAY_CURRENT] = ret->addDisplay("Current");
    dataDisplays[DISPLAY_DIRECTION] = ret->addDisplay("Direction");
    
    return ret;
}

CDisplayWidget *CNCursClient::createSensorDisplay()
{
    CDisplayWidget *ret = new CDisplayWidget("Sensors");

    dataDisplays[DISPLAY_LIGHT] = ret->addDisplay("Light");
    dataDisplays[DISPLAY_ACS] = ret->addDisplay("ACS");
    dataDisplays[DISPLAY_BUMPERS] = ret->addDisplay("Bumpers");
    
    return ret;
}

CDisplayWidget *CNCursClient::createOtherDisplay()
{
    CDisplayWidget *ret = new CDisplayWidget("Other");

    dataDisplays[DISPLAY_BATTERY] = ret->addDisplay("Battery");
    dataDisplays[DISPLAY_RC5] = ret->addDisplay("RC5");
    dataDisplays[DISPLAY_MAIN_LEDS] = ret->addDisplay("Main LEDs");
    dataDisplays[DISPLAY_M32_LEDS] = ret->addDisplay("M32 LEDs");
    dataDisplays[DISPLAY_KEYS] = ret->addDisplay("Pressed keys");
    
    return ret;
}

void CNCursClient::updateConnection(bool connected)
{
    if (connected)
    {
        connectButton->SetText("Disconnect");
        appendLogText("Disconnected from server.\n");
    }
    else
    {
        connectButton->SetText("Connect");
        appendLogText("Connected to server.\n");
    }
}

void CNCursClient::parseTcp(QDataStream &stream)
{
    QString msg;
    QVariant data;
    stream >> msg >> data;

    if (msg == "lightleft")
    {
        dataDisplays[DISPLAY_LIGHT]->SetText(data.toString().toStdString());
    }
}

void CNCursClient::connectedToServer()
{
    updateConnection(true);
}

void CNCursClient::disconnectedFromServer()
{
    updateConnection(false);
}

void CNCursClient::serverHasData()
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
        
        if (clientSocket->bytesAvailable() < tcpReadBlockSize)
            return;
        
        parseTcp(in);
        tcpReadBlockSize = 0;
    }
}

void CNCursClient::socketError(QAbstractSocket::SocketError)
{
    // ...
    updateConnection(false);
}

bool CNCursClient::CoreHandleEvent(NNCurses::CWidget *emitter, int type)
{
    if (NNCurses::CWindow::CoreHandleEvent(emitter, type))
        return true;
    
    if (type == EVENT_CALLBACK)
    {
        if (emitter == connectButton)
        {
            const bool wasconnected = (clientSocket->state() ==
                QAbstractSocket::ConnectedState);
            clientSocket->abort(); // Always disconnect first

            if (!wasconnected)
                clientSocket->connectToHost("localhost", 40000);
            
            return true;
        }
    }
    
    return false;
}

bool CNCursClient::CoreHandleKey(wchar_t key)
{
    if (NNCurses::CWindow::CoreHandleKey(key))
        return true;
    
    if (key == KEY_F(2))
    {
        enableScreen(mainScreen);
        return true;
    }
    else if (key == KEY_F(3))
    {
        enableScreen(logScreen);
        return true;
    }
    
    return false;
}

void CNCursClient::CoreGetButtonDescs(NNCurses::TButtonDescList &list)
{
    list.push_back(NNCurses::TButtonDescPair("F2", "Main"));
    list.push_back(NNCurses::TButtonDescPair("F3", "Log"));
    CWindow::CoreGetButtonDescs(list);
}
