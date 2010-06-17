#include <sstream>

#include <QtCore>
#include <QtNetwork>

#include "tui/button.h"
#include "tui/label.h"
#include "tui/textfield.h"
#include "tui/tui.h"
#include "client.h"
#include "display_widget.h"
#include "tcputil.h"

namespace {

std::string intToStdString(int n)
{
    std::stringstream stream;
    stream << n;
    return stream.str();
}

std::string binaryToStdString(uint8_t n)
{
    return ((QString()).setNum(n, 2).toStdString());
}

std::string directionToStdString(uint8_t dir)
{
    switch (dir)
    {
        case FWD: return "fwd";
        case BWD: return "bwd";
        case LEFT: return "left";
        case RIGHT: return "right";
    }
    
    return std::string();
}

}

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


CNCursClient::CNCursClient() : activeScreen(NULL),
                               drivingEnabled(false)
{
    StartPack(mainScreen = createMainScreen(), false, true, 0, 0);
    StartPack(consoleScreen = createConsoleScreen(), false, true, 0, 0);
    StartPack(logScreen = createLogScreen(), false, true, 0, 0);

    screenList.push_back(mainScreen);
    screenList.push_back(consoleScreen);
    screenList.push_back(logScreen);

    enableScreen(mainScreen);

    appendLogOutput("Started NCurses client.\n");
}

CNCursClient::TScreen *CNCursClient::createMainScreen()
{
    TScreen *ret = new TScreen(CBox::VERTICAL, false);
    ret->Enable(false);

    NNCurses::CBox *hbox = new NNCurses::CBox(CBox::HORIZONTAL, false);
    ret->AddWidget(hbox);
    
    hbox->AddWidget(movementDisplay = createMovementDisplay());
    hbox->AddWidget(sensorDisplay = createSensorDisplay());
    hbox->AddWidget(otherDisplay = createOtherDisplay());
    
    ret->EndPack(connectButton = new NNCurses::CButton("Connect"), false, true, 0, 0);
    
    return ret;
}

CNCursClient::TScreen *CNCursClient::createConsoleScreen()
{
    TScreen *ret = new TScreen(CBox::VERTICAL, true);
    ret->Enable(false);
    
    ret->AddWidget(consoleOutput = new NNCurses::CTextField(30, 10, true));
    
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

CDisplayWidget *CNCursClient::createMovementDisplay()
{
    CDisplayWidget *ret = new CDisplayWidget("Movement");

    ret->addDisplay("Speed", DISPLAY_SPEED, 2);
    ret->addDisplay("Distance", DISPLAY_DISTANCE, 2);
    ret->addDisplay("Current", DISPLAY_CURRENT, 2);
    ret->addDisplay("Direction", DISPLAY_DIRECTION, 2);
    
    return ret;
}

CDisplayWidget *CNCursClient::createSensorDisplay()
{
    CDisplayWidget *ret = new CDisplayWidget("Sensors");

    ret->addDisplay("Light", DISPLAY_LIGHT, 2);
    ret->addDisplay("ACS", DISPLAY_ACS, 2);
    ret->addDisplay("Bumpers", DISPLAY_BUMPERS, 2);
    
    return ret;
}

CDisplayWidget *CNCursClient::createOtherDisplay()
{
    CDisplayWidget *ret = new CDisplayWidget("Other");

    ret->addDisplay("Battery", DISPLAY_BATTERY, 1);
    ret->addDisplay("RC5 (dev/tog/key)", DISPLAY_RC5, 3);
    ret->addDisplay("Main LEDs", DISPLAY_MAIN_LEDS, 1);
    ret->addDisplay("M32 LEDs", DISPLAY_M32_LEDS, 1);
    ret->addDisplay("Pressed keys", DISPLAY_KEYS, 1);
    
    return ret;
}

void CNCursClient::updateConnection(bool connected)
{
    connectButton->SetText((connected) ? "Disconnect" : "Connect");
}

void CNCursClient::tcpRobotStateUpdate(const SStateSensors &,
                                       const SStateSensors &newstate)
{
    sensorDisplay->setDisplayValue(DISPLAY_ACS, 0,
                                   intToStdString(newstate.ACSLeft));
    sensorDisplay->setDisplayValue(DISPLAY_ACS, 1,
                                   intToStdString(newstate.ACSRight));

    sensorDisplay->setDisplayValue(DISPLAY_BUMPERS, 0,
                                   intToStdString(newstate.bumperLeft));
    sensorDisplay->setDisplayValue(DISPLAY_BUMPERS, 1,
                                   intToStdString(newstate.bumperRight));
}

void CNCursClient::tcpHandleRobotData(ETcpMessage msg, int data)
{
    if (msg == TCP_BASE_LEDS)
        otherDisplay->setDisplayValue(DISPLAY_MAIN_LEDS, 0,
                                        binaryToStdString(data));
    else if (msg == TCP_M32_LEDS)
        otherDisplay->setDisplayValue(DISPLAY_M32_LEDS, 0,
                                        binaryToStdString(data));
    else if (msg == TCP_LIGHT_LEFT)
        sensorDisplay->setDisplayValue(DISPLAY_LIGHT, 0, intToStdString(data));
    else if (msg == TCP_LIGHT_RIGHT)
        sensorDisplay->setDisplayValue(DISPLAY_LIGHT, 1, intToStdString(data));
    else if (msg == TCP_MOTOR_SPEED_LEFT)
        movementDisplay->setDisplayValue(DISPLAY_SPEED, 0, intToStdString(data));
    else if (msg == TCP_MOTOR_SPEED_RIGHT)
        movementDisplay->setDisplayValue(DISPLAY_SPEED, 1, intToStdString(data));
    else if (msg == TCP_MOTOR_DIST_LEFT)
        movementDisplay->setDisplayValue(DISPLAY_DISTANCE, 0, intToStdString(data));
    else if (msg == TCP_MOTOR_DIST_RIGHT)
        movementDisplay->setDisplayValue(DISPLAY_DISTANCE, 1, intToStdString(data));
    else if (msg == TCP_MOTOR_CURRENT_LEFT)
        movementDisplay->setDisplayValue(DISPLAY_CURRENT, 0, intToStdString(data));
    else if (msg == TCP_MOTOR_CURRENT_RIGHT)
        movementDisplay->setDisplayValue(DISPLAY_CURRENT, 1, intToStdString(data));
    else if (msg == TCP_MOTOR_DIRECTIONS)
    {
        SMotorDirections dir;
        dir.byte = data;
        movementDisplay->setDisplayValue(DISPLAY_DIRECTION, 0,
                                            directionToStdString(dir.left));
        movementDisplay->setDisplayValue(DISPLAY_DIRECTION, 1,
                                            directionToStdString(dir.right));
    }
    else if (msg == TCP_BATTERY)
        otherDisplay->setDisplayValue(DISPLAY_BATTERY, 0, intToStdString(data));
    else if (msg == TCP_LASTRC5)
    {
        RC5data_t rc5;
        rc5.data = data;
        otherDisplay->setDisplayValue(DISPLAY_RC5, 0,
                                            intToStdString(rc5.device));
        otherDisplay->setDisplayValue(DISPLAY_RC5, 1,
                                            intToStdString(rc5.toggle_bit));
        otherDisplay->setDisplayValue(DISPLAY_RC5, 2,
                                            intToStdString(rc5.key_code));
    }
}

void CNCursClient::appendConsoleOutput(const QString &text)
{
    consoleOutput->AddText(text.toStdString());
}

void CNCursClient::appendLogOutput(const QString &text)
{
    std::string s = (QTime::currentTime().toString() + ": " + text).toStdString();
    logWidget->AddText(s);
}

bool CNCursClient::CoreHandleEvent(NNCurses::CWidget *emitter, int type)
{
    if (NNCurses::CWindow::CoreHandleEvent(emitter, type))
        return true;
    
    if (type == EVENT_CALLBACK)
    {
        if (emitter == connectButton)
        {
            if (connected())
                disconnectFromServer();
            else
                connectToHost("localhost");
            
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
        enableScreen(consoleScreen);
        return true;
    }
    else if (key == KEY_F(4))
    {
        enableScreen(logScreen);
        return true;
    }
    else if (key == KEY_F(5))
    {
        drivingEnabled = !drivingEnabled;
        if (!drivingEnabled)
            stopDrive();
        return true;
    }
    else if ((activeScreen == mainScreen) && drivingEnabled)
    {
        switch (key)
        {
            case KEY_UP: updateDriving(FWD); return true;
            case KEY_LEFT: updateDriving(LEFT); return true;
            case KEY_RIGHT: updateDriving(RIGHT); return true;
            case KEY_DOWN: updateDriving(BWD); return true;
        }
    }
    
    return false;
}

void CNCursClient::CoreGetButtonDescs(NNCurses::TButtonDescList &list)
{
    list.push_back(NNCurses::TButtonDescPair("F2", "Main"));
    list.push_back(NNCurses::TButtonDescPair("F3", "Console"));
    list.push_back(NNCurses::TButtonDescPair("F4", "Log"));
    list.push_back(NNCurses::TButtonDescPair("F5", "Toggle drive"));
    CWindow::CoreGetButtonDescs(list);
}
