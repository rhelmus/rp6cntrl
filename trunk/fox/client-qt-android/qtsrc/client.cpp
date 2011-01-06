#include <QtGui>

#include "client.h"
#include "drivewidget.h"
#include "statwidget.h"

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

CQtClient::CQtClient()
{
    QTabWidget *tabw = new QTabWidget;
    setCentralWidget(tabw);

    tabw->addTab(createConnectTab(), "Connect");
    tabw->addTab(createStatTab(), "Status");
    tabw->addTab(createConsoleTab(), "Console");
    tabw->addTab(createLogTab(), "Log");
    tabw->addTab(createDriveTab(), "Drive");
}

QWidget *CQtClient::createConnectTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    vbox->addWidget(serverEdit = new QLineEdit("192.168.1.40"));
    vbox->addWidget(connectButton = new QPushButton("Connect"));
    connect(connectButton, SIGNAL(clicked()), this,
            SLOT(toggleServerConnection()));

    return ret;
}

QWidget *CQtClient::createStatTab()
{
    QScrollArea *ret = new QScrollArea;
    ret->setWidgetResizable(true);
    flickCharm.activateOn(ret);

    QWidget *sw = new QWidget;
    ret->setWidget(sw);
    QVBoxLayout *vbox = new QVBoxLayout(sw);

    QFrame *frame = new QFrame;
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    vbox->addWidget(frame);

    QVBoxLayout *subvbox = new QVBoxLayout(frame);
    QLabel *l = new QLabel;
    l->setText("<qt><b>Movement</b>");
    l->setAlignment(Qt::AlignCenter);
    subvbox->addWidget(l);
    subvbox->addWidget(statWidgets[DISPLAY_SPEED] = new CStatWidget("Speed", 2));
    subvbox->addWidget(statWidgets[DISPLAY_DISTANCE] = new CStatWidget("Distance", 2));
    subvbox->addWidget(statWidgets[DISPLAY_CURRENT] = new CStatWidget("Current", 2));
    subvbox->addWidget(statWidgets[DISPLAY_DIRECTION] = new CStatWidget("Direction", 2));

    frame = new QFrame;
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    vbox->addWidget(frame);

    subvbox = new QVBoxLayout(frame);
    l = new QLabel;
    l->setText("<qt><b>Sensors</b>");
    l->setAlignment(Qt::AlignCenter);
    subvbox->addWidget(l);
    subvbox->addWidget(statWidgets[DISPLAY_LIGHT] = new CStatWidget("Light", 2));
    subvbox->addWidget(statWidgets[DISPLAY_ACS] = new CStatWidget("ACS", 2));
    subvbox->addWidget(statWidgets[DISPLAY_BUMPERS] = new CStatWidget("Bumpers", 2));
    subvbox->addWidget(statWidgets[DISPLAY_SHARPIR] = new CStatWidget("Sharp IR", 1));

    frame = new QFrame;
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    vbox->addWidget(frame);

    subvbox = new QVBoxLayout(frame);
    l = new QLabel;
    l->setText("<qt><b>Other</b>");
    l->setAlignment(Qt::AlignCenter);
    subvbox->addWidget(l);
    subvbox->addWidget(statWidgets[DISPLAY_BATTERY] = new CStatWidget("Battery", 1));
    subvbox->addWidget(statWidgets[DISPLAY_RC5] = new CStatWidget("RC5", 3));
    subvbox->addWidget(statWidgets[DISPLAY_MAIN_LEDS] = new CStatWidget("Main LEDs", 6));
    subvbox->addWidget(statWidgets[DISPLAY_M32_LEDS] = new CStatWidget("M32 LEDs", 4));
    subvbox->addWidget(statWidgets[DISPLAY_KEYS] = new CStatWidget("Pressed keys", 5));

    return ret;
}

QWidget *CQtClient::createConsoleTab()
{
    QWidget *ret = new QWidget;

    QVBoxLayout *vbox = new QVBoxLayout(ret);

    vbox->addWidget(consoleOut = new QPlainTextEdit);
    consoleOut->setReadOnly(true);
    consoleOut->setCenterOnScroll(true);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);

    hbox->addWidget(consoleIn = new QLineEdit);

    QPushButton *button = new QPushButton("Send");
    connect(button, SIGNAL(clicked()), this, SLOT(sendConsolePressed()));
    connect(consoleIn, SIGNAL(returnPressed()), button, SLOT(click()));
    hbox->addWidget(button);

    button = new QPushButton("Clear console");
    connect(button, SIGNAL(clicked()), consoleOut, SLOT(clear()));
    hbox->addWidget(button);

    return ret;
}

QWidget *CQtClient::createLogTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    vbox->addWidget(logWidget = new QPlainTextEdit);
    logWidget->setReadOnly(true);
    logWidget->setCenterOnScroll(true);

    QPushButton *button = new QPushButton("Clear log");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(button, SIGNAL(clicked()), logWidget, SLOT(clear()));
    vbox->addWidget(button, 0, Qt::AlignCenter);

    return ret;
}

QWidget *CQtClient::createDriveTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    vbox->addWidget(new CDriveWidget);

    return ret;
}

void CQtClient::updateConnection(bool connected)
{
    connectButton->setText((connected) ? "Disconnect" : "Connect");
}

void CQtClient::tcpError(const QString &error)
{
    if (QMessageBox::critical(this, "Socket error", error,
                              QMessageBox::Retry | QMessageBox::Cancel) == QMessageBox::Retry)
        QTimer::singleShot(100, connectButton, SLOT(click()));
}

void CQtClient::tcpRobotStateUpdate(const SStateSensors &,
                                    const SStateSensors &newstate)
{
    statWidgets[DISPLAY_ACS]->setValue(0, (newstate.ACSLeft) ? "X" : "");
    statWidgets[DISPLAY_ACS]->setValue(1, (newstate.ACSRight) ? "X" : "");
    statWidgets[DISPLAY_BUMPERS]->setValue(0, (newstate.bumperLeft) ? "X" : "");
    statWidgets[DISPLAY_BUMPERS]->setValue(1, (newstate.bumperRight) ? "X" : "");
}

void CQtClient::tcpHandleRobotData(ETcpMessage msg, int data)
{
    switch (msg)
    {
    case TCP_MOTOR_SPEED_LEFT:
        statWidgets[DISPLAY_SPEED]->setValue(0, data);
        break;
    case TCP_MOTOR_SPEED_RIGHT:
        statWidgets[DISPLAY_SPEED]->setValue(1, data);
        break;
    case TCP_MOTOR_DIST_LEFT:
        statWidgets[DISPLAY_DISTANCE]->setValue(0, data);
        break;
    case TCP_MOTOR_DIST_RIGHT:
        statWidgets[DISPLAY_DISTANCE]->setValue(1, data);
        break;
    case TCP_MOTOR_CURRENT_LEFT:
        statWidgets[DISPLAY_CURRENT]->setValue(0, data);
        break;
    case TCP_MOTOR_CURRENT_RIGHT:
        statWidgets[DISPLAY_CURRENT]->setValue(1, data);
        break;
    case TCP_MOTOR_DIRECTIONS:
        SMotorDirections dir;
        dir.byte = data;
        statWidgets[DISPLAY_DIRECTION]->setValue(0, directionString(dir.left));
        statWidgets[DISPLAY_DIRECTION]->setValue(1, directionString(dir.right));
        break;

    case TCP_LIGHT_LEFT:
        statWidgets[DISPLAY_LIGHT]->setValue(0, data);
        break;
    case TCP_LIGHT_RIGHT:
        statWidgets[DISPLAY_LIGHT]->setValue(1, data);
        break;
    case TCP_SHARPIR:
        statWidgets[DISPLAY_SHARPIR]->setValue(0, data);
        break;

    case TCP_BATTERY:
        statWidgets[DISPLAY_BATTERY]->setValue(0, data);
        break;

    case TCP_LASTRC5:
        {
            RC5data_t rc5;
            rc5.data = data;
            statWidgets[DISPLAY_RC5]->setValue(0, rc5.device);
            statWidgets[DISPLAY_RC5]->setValue(1, rc5.toggle_bit);
            statWidgets[DISPLAY_RC5]->setValue(2, rc5.key_code);
            break;
        }

    case TCP_BASE_LEDS:
        for (int i=0; i<6; ++i)
        {
            QString v = (data & (1<<i)) ? "o" : "";
            statWidgets[DISPLAY_MAIN_LEDS]->setValue(i, v);
        }
        break;
    case TCP_M32_LEDS:
        for (int i=0; i<4; ++i)
        {
            QString v = (data & (1<<i)) ? "o" : "";
            statWidgets[DISPLAY_M32_LEDS]->setValue(i, v);
        }
        break;

    default: break;
    }
}

void CQtClient::toggleServerConnection()
{
    if (connected())
        disconnectFromServer();
    else
        connectToHost(serverEdit->text());
}

void CQtClient::sendConsolePressed()
{
    // Append cmd
    appendConsoleOutput(QString("> %1\n").arg(consoleIn->text()));
    executeCommand(consoleIn->text());
    consoleIn->clear();
}

void CQtClient::appendConsoleOutput(const QString &text)
{
    consoleOut->appendPlainText(text);
}

void CQtClient::appendLogOutput(const QString &text)
{
    logWidget->appendHtml(QString("<FONT color=#FF0000><strong>[%1]</strong></FONT> %2")
            .arg(QTime::currentTime().toString()).arg(text));
}
