#include <QtCore>
#include <QtNetwork>

#include "tui/button.h"
#include "tui/label.h"
#include "tui/separator.h"
#include "tui/tui.h"
#include "client.h"

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


CNCursClient::CNCursClient() : tcpReadBlockSize(0), activeScreen(NULL)
{
    initDisplayMaps();
    
    StartPack(mainScreen = createMainScreen(), false, true, 0, 0);
    StartPack(logScreen = createLogScreen(), false, true, 0, 0);

    screenList.push_back(mainScreen);
    screenList.push_back(logScreen);

    enableScreen(mainScreen);
}

CNCursClient::TScreen *CNCursClient::createMainScreen()
{
    TScreen *ret = new TScreen(CBox::VERTICAL, false);
    ret->Enable(false);

    NNCurses::CBox *hbox = new NNCurses::CBox(CBox::HORIZONTAL, true);
    ret->AddWidget(hbox);
    
    hbox->AddWidget(createDisplayWidget("Movement", movementDisplays));
    hbox->AddWidget(createDisplayWidget("Sensors", sensorDisplays));
    hbox->AddWidget(createDisplayWidget("Other", otherDisplays));
    
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

    ret->AddWidget(new NNCurses::CLabel("Placeholder here"));
    
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

void CNCursClient::initDisplayMaps()
{
    movementDisplays["Speed"] = NULL;
    movementDisplays["Distance"] = NULL;
    movementDisplays["Current"] = NULL;
    movementDisplays["Direction"] = NULL;
    
    sensorDisplays["Light"] = NULL;
    sensorDisplays["ACS"] = NULL;
    sensorDisplays["Bumpers"] = NULL;
    
    otherDisplays["Battery"] = NULL;
    otherDisplays["RC5"] = NULL;
    otherDisplays["Main LEDs"] = NULL;
    otherDisplays["M32 LEDs"] = NULL;
    otherDisplays["Pressed key"] = NULL;
}

NNCurses::CBox *CNCursClient::createDisplayWidget(const std::string &title,
                                                 TDisplayMap &map)
{
    NNCurses::CBox *ret = new NNCurses::CBox(CBox::VERTICAL, false);
    ret->SetBox(true);
    ret->SetMinWidth(10);

    ret->StartPack(new NNCurses::CLabel(title), false, true, 0, 0);
    ret->StartPack(new NNCurses::CSeparator(ACS_HLINE), false, true, 0, 0);

    NNCurses::CBox *hbox = new NNCurses::CBox(CBox::HORIZONTAL, false, 1);
    ret->StartPack(hbox, false, false, 0, 0);

    NNCurses::CBox *keyvbox = new NNCurses::CBox(CBox::VERTICAL, false);
    hbox->StartPack(keyvbox, false, false, 0, 0);

    NNCurses::CBox *datavbox = new NNCurses::CBox(CBox::VERTICAL, false);
    hbox->StartPack(datavbox, true, true, 0, 0);
    datavbox->SetDFColors(COLOR_YELLOW, COLOR_BLUE);

    for (TDisplayMap::iterator it=map.begin(); it!=map.end(); ++it)
    {
        keyvbox->StartPack(new NNCurses::CLabel(it->first, false), false, false, 0, 0);
        datavbox->StartPack(it->second = new NNCurses::CLabel("NA"), true,
                            true, 0, 0);
    }
    
    return ret;
}

void CNCursClient::updateConnection(bool connected)
{
    if (connected)
        connectButton->SetText("Disconnect");
    else
        connectButton->SetText("Connect");
}

void CNCursClient::parseTcp(QDataStream &stream)
{
    QString msg;
    QVariant data;
    stream >> msg >> data;
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

void CNCursClient::socketError(QAbstractSocket::SocketError error)
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
