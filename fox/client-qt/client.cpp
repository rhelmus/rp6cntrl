#include <stdint.h>

#include <QtGui>
#include <QtNetwork>

#include <qwt_slider.h>

#include "client.h"
#include "scanner.h"
#include "sensorplot.h"
#include "tcputil.h"

namespace {

QLabel *createDataLabel(const QString &l)
{
    QLabel *ret = new QLabel(l);
    ret->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    return ret;
}

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

QString ACSStateToString(uint8_t state)
{
    switch (state)
    {
        case ACS_STATE_IDLE: return "idle";
        case ACS_STATE_IRCOMM_DELAY: return "checking for incoming ircomm";
        case ACS_STATE_SEND_LEFT: return "sending left pulses";
        case ACS_STATE_WAIT_LEFT: return "receiving left pulses";
        case ACS_STATE_SEND_RIGHT: return "sending right pulses";
        case ACS_STATE_WAIT_RIGHT: return "receiving right pulses";
    }
    
    return QString();
}

QString ACSPowerToString(EACSPowerState state)
{
    switch (state)
    {
        case ACS_POWER_OFF: return "off";
        case ACS_POWER_LOW: return "low";
        case ACS_POWER_MED: return "med";
        case ACS_POWER_HIGH: return "high";
    }
    
    return QString();
}

QPushButton *createDriveButton(const QIcon &icon, Qt::Key key)
{
    QPushButton *ret = new QPushButton;
    ret->setIcon(icon);
    ret->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ret->setShortcut(key);
    return ret;
}

QWidget *createSlider(const QString &title, QwtSlider *&slider, int min, int max)
{
    QFrame *ret = new QFrame;
    ret->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    ret->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QLabel *label = new QLabel(title);
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);

    vbox->addWidget(slider = new QwtSlider(0, Qt::Vertical, QwtSlider::LeftScale));
    slider->setScale(min, max);
    slider->setRange(min, max);
    slider->setReadOnly(true);
    
    return ret;
}

}


CQtClient::CQtClient() : tcpReadBlockSize(0), currentDriveDirection(FWD),
                     driveTurning(false), driveForwardSpeed(0), driveTurnSpeed(0),
                     isScanning(false), alternatingScan(false),
                     remainingACSCycles(0), firstStateUpdate(false),
                     ACSPowerState(ACS_POWER_OFF)
{
    QTabWidget *mainTab = new QTabWidget;
    mainTab->setTabPosition(QTabWidget::West);
    setCentralWidget(mainTab);
    
    mainTab->addTab(createMainTab(), "Main");
    mainTab->addTab(createConsoleTab(), "Console");
    mainTab->addTab(createLogTab(), "Log");
    
    clientSocket = new QTcpSocket(this);
    connect(clientSocket, SIGNAL(connected()), this, SLOT(connectedToServer()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    
    QTimer *uptimer = new QTimer(this);
    connect(uptimer, SIGNAL(timeout()), this, SLOT(updateSensors()));
    uptimer->start(500);
    
    currentStateSensors.byte = 0;
    motorDistance[0] = motorDistance[1] = 0;
    destMotorDistance[0] = destMotorDistance[1] = 0;
    currentMotorDirections.byte = 0;
    
    appendLogText("Started RP6 Qt frontend.\n");
}

QWidget *CQtClient::createMainTab()
{
    QWidget *ret = new QWidget;
    
    QVBoxLayout *vbox = new QVBoxLayout(ret);
    
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    vbox->addWidget(splitter);
    
    QTabWidget *tabWidget = new QTabWidget;
    splitter->addWidget(tabWidget);
    tabWidget->addTab(createOverviewWidget(), "Overview");
    tabWidget->addTab(createSpeedWidget(), "Motor speed");
    tabWidget->addTab(createDistWidget(), "Motor distance");
    tabWidget->addTab(createCurrentWidget(), "Motor current");
    tabWidget->addTab(createLightWidget(), "Light sensors");
    tabWidget->addTab(createACSWidget(), "ACS");
    tabWidget->addTab(createBatteryWidget(), "Battery");
    tabWidget->addTab(createMicWidget(), "Microphone");

    splitter->addWidget(tabWidget = new QTabWidget);
    tabWidget->addTab(createConnectionWidget(), "Connection");
    tabWidget->addTab(createDriveWidget(), "Drive");
    tabWidget->addTab(createScannerWidget(), "Scan");
    
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
    connect(button, SIGNAL(clicked()), this, SLOT(sendConsoleCommand()));
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

QWidget *CQtClient::createOverviewWidget(void)
{
    QWidget *ret = new QWidget;
    
    QGridLayout *grid = new QGridLayout(ret);
    
    // Motor group box
    // UNDONE: destination speed, distance
    QGroupBox *group = new QGroupBox("Motor");
    grid->addWidget(group, 0, 0);
    QFormLayout *form = new QFormLayout(group);
    
    QWidget *w = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(w);
    hbox->addWidget(motorSpeedLCD[0] = new QLCDNumber);
    hbox->addWidget(motorSpeedLCD[1] = new QLCDNumber);
    form->addRow("Speed", w);
    
    form->addRow("Distance", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(motorDistanceLCD[0] = new QLCDNumber);
    hbox->addWidget(motorDistanceLCD[1] = new QLCDNumber);

    form->addRow("Current", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(motorCurrentLCD[0] = new QLCDNumber);
    hbox->addWidget(motorCurrentLCD[1] = new QLCDNumber);

    form->addRow("Direction", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(motorDirection[0] = createDataLabel("<none>"));
    hbox->addWidget(motorDirection[1] = createDataLabel("<none>"));

    
    // Light sensors group box
    group = new QGroupBox("Optics");
    grid->addWidget(group, 0, 1);
    form = new QFormLayout(group);

    form->addRow("Light sensors", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(lightSensorsLCD[0] = new QLCDNumber);
    hbox->addWidget(lightSensorsLCD[1] = new QLCDNumber);
    
    // RC5 group box
    group = new QGroupBox("RC5 remote control");
    grid->addWidget(group, 0, 2);
    form = new QFormLayout(group);

    form->addRow("Device", RC5DeviceLabel = createDataLabel("<none>"));
    form->addRow("Toggle bit", RC5ToggleBitBox = new QCheckBox);
    RC5ToggleBitBox->setEnabled(false);
    form->addRow("Key", RC5KeyLabel = createDataLabel("<none>"));

    
    // ACS group box
    group = new QGroupBox("ACS");
    grid->addWidget(group, 1, 0);
    form = new QFormLayout(group);

    form->addRow("ACS", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(ACSCollisionBox[0] = new QCheckBox);
    hbox->addWidget(ACSCollisionBox[1] = new QCheckBox);
    ACSCollisionBox[0]->setEnabled(false);
    ACSCollisionBox[1]->setEnabled(false);
    
    form->addRow("Power state", ACSPowerSlider = new QSlider(Qt::Horizontal));
    ACSPowerSlider->setRange(0, 3);
    ACSPowerSlider->setEnabled(false);
    ACSPowerSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    
    // LEDs group box
    group = new QGroupBox("LEDs");
    grid->addWidget(group, 1, 1);
    form = new QFormLayout(group);
    
    form->addRow("Main LEDs", w = new QWidget);
    QGridLayout *ledgrid = new QGridLayout(w);
    
    for (int col=0; col<2; ++col)
    {
        for (int row=0; row<3; ++row)
        {
            int ind = row + (col * 3);
            ledgrid->addWidget(mainLEDsBox[ind] = new QCheckBox, row, col);
            mainLEDsBox[ind]->setEnabled(false);
        }
    }
    
    form->addRow("M32 LEDs", w = new QWidget);
    hbox = new QHBoxLayout(w);
    for (int i=0; i<4; ++i)
    {
        hbox->addWidget(m32LEDsBox[i] = new QCheckBox);
        m32LEDsBox[i]->setEnabled(false);
    }

    
    // Others group box
    group = new QGroupBox("Other");
    grid->addWidget(group, 1, 2);
    form = new QFormLayout(group);

    form->addRow("Battery", batteryLCD = new QLCDNumber);
    
    form->addRow("Bumpers", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(bumperBox[0] = new QCheckBox);
    hbox->addWidget(bumperBox[1] = new QCheckBox);
    bumperBox[0]->setEnabled(false);
    bumperBox[1]->setEnabled(false);
    
    form->addRow("Pressed key", m32KeyLabel = createDataLabel("<none>"));

    return ret;
}

QWidget *CQtClient::createSpeedWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    motorSpeedPlot = new CSensorPlot("Motor speed");
    motorSpeedPlot->addSensor("Left (actual)", Qt::red);
    motorSpeedPlot->addSensor("Right (actual)", Qt::blue);
    motorSpeedPlot->addSensor("Left (destination)", Qt::blue);
    motorSpeedPlot->addSensor("Right (destination)", Qt::darkBlue);

    hbox->addWidget(motorSpeedPlot);
    
    return ret;
}

QWidget *CQtClient::createDistWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    motorDistancePlot = new CSensorPlot("Motor distance");
    motorDistancePlot->addSensor("Left", Qt::red);
    motorDistancePlot->addSensor("Right", Qt::blue);

    hbox->addWidget(motorDistancePlot);

    return ret;
}

QWidget *CQtClient::createCurrentWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    motorCurrentPlot = new CSensorPlot("Motor current");
    motorCurrentPlot->addSensor("Left", Qt::red);
    motorCurrentPlot->addSensor("Right", Qt::blue);

    hbox->addWidget(motorCurrentPlot);

    return ret;
}

QWidget *CQtClient::createLightWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    lightSensorsPlot = new CSensorPlot("Light sensors");
    lightSensorsPlot->addSensor("Left", Qt::red);
    lightSensorsPlot->addSensor("Right", Qt::blue);

    hbox->addWidget(lightSensorsPlot);

    return ret;
}

QWidget *CQtClient::createACSWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    ACSPlot = new CSensorPlot("Anti Collision Sensors");
    ACSPlot->addSensor("Left", Qt::red, QwtPlotCurve::Steps);
    ACSPlot->addSensor("Right", Qt::blue, QwtPlotCurve::Steps);

    hbox->addWidget(ACSPlot);

    return ret;
}

QWidget *CQtClient::createBatteryWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    batteryPlot = new CSensorPlot("Battery voltage");
    batteryPlot->addSensor("Battery", Qt::red);

    hbox->addWidget(batteryPlot);
    
    return ret;
}

QWidget *CQtClient::createMicWidget(void)
{
    QWidget *ret = new QWidget;
    
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    micPlot = new CSensorPlot("Microphone");
    micPlot->addSensor("Microphone", Qt::red);

    vbox->addWidget(micPlot);
    
    QWidget *w = new QWidget;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    vbox->addWidget(w, 0, Qt::AlignCenter);
    
    QHBoxLayout *hbox = new QHBoxLayout(w);
    
    hbox->addWidget(micUpdateTimeSpinBox = new QSpinBox);
    micUpdateTimeSpinBox->setSuffix(" ms");
    micUpdateTimeSpinBox->setRange(0, 100000);
    micUpdateTimeSpinBox->setValue(500);
    micUpdateTimeSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(micUpdateTimeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setMicUpdateTime(int)));
    
    hbox->addWidget(micUpdateToggleButton = new QPushButton("Record..."));
    micUpdateToggleButton->setCheckable(true);
    micUpdateToggleButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(micUpdateToggleButton, SIGNAL(toggled(bool)), this, SLOT(micPlotToggled(bool)));
    
    return ret;
}

QWidget *CQtClient::createConnectionWidget()
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    hbox->addWidget(serverEdit = new QLineEdit);
    serverEdit->setText("localhost");
    
    QPushButton *button = new QPushButton("Connect");
    connect(button, SIGNAL(clicked()), this, SLOT(connectToServer()));
    hbox->addWidget(button);
    
    return ret;
}

QWidget *CQtClient::createDriveWidget()
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    // Arrow box
    QWidget *w = new QWidget;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(w);
    
    QGridLayout *grid = new QGridLayout(w);
    
    driveMapper = new QSignalMapper(this);
    connect(driveMapper, SIGNAL(mapped(int)), this, SLOT(driveButtonPressed(int)));
    
    QPushButton *button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowUp), Qt::Key_Up);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, FWD);
    grid->addWidget(button, 0, 1);
    
    button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowLeft), Qt::Key_Left);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, LEFT);
    grid->addWidget(button, 1, 0);
    
    button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowDown), Qt::Key_Down);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, BWD);
    grid->addWidget(button, 1, 1);

    button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowRight), Qt::Key_Right);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, RIGHT);
    grid->addWidget(button, 1, 2);
    
    // Others
    button = new QPushButton("Full stop");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(button, SIGNAL(clicked()), this, SLOT(stopDriveButtonPressed()));
    hbox->addWidget(button);
    
    hbox->addWidget(createSlider("<qt><strong>Left</strong>", driveSpeedSlider[0], -255, 255));
    hbox->addWidget(createSlider("<qt><strong>Right</strong>", driveSpeedSlider[1], -255, 255));
    
    return ret;
}

QWidget *CQtClient::createScannerWidget()
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    QWidget *w = new QWidget;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(w, 0, Qt::AlignVCenter);
    QFormLayout *form = new QFormLayout(w);
    
    form->addRow("Speed", scanSpeedSpinBox = new QSpinBox);
    scanSpeedSpinBox->setRange(0, 160);
    scanSpeedSpinBox->setValue(60);

    form->addRow("Scan power", scanPowerComboBox = new QComboBox);
    scanPowerComboBox->addItems(QStringList() << "Low" << "Medium" << "High" <<
            "Alternating");
    scanPowerComboBox->setCurrentIndex(1);
    
    form->addRow(scanButton = new QPushButton("Scan"));
    connect(scanButton, SIGNAL(clicked()), this, SLOT(startScan()));
    
    
    hbox->addWidget(scannerWidget = new CScannerWidget, 0);

    return ret;
}

void CQtClient::appendConsoleText(const QString &text)
{
    QTextCursor cur = consoleOut->textCursor();
    cur.movePosition(QTextCursor::End);
    consoleOut->setTextCursor(cur);
    consoleOut->insertPlainText(text);
}

void CQtClient::appendLogText(const QString &text)
{
    logWidget->appendHtml(QString("<FONT color=#FF0000><strong>[%1]</strong></FONT> %2")
            .arg(QTime::currentTime().toString()).arg(text));
/*    QTextCursor cur = logWidget->textCursor();
    cur.movePosition(QTextCursor::End);
    logWidget->setTextCursor(cur);
    logWidget->insertPlainText(text);*/
}

void CQtClient::parseTcp(QDataStream &stream)
{
    QString msg;
    stream >> msg;

    if (msg == "rawserial")
    {
        QString output;
        stream >> output;
        appendConsoleText(output + "\n");
    }
    else if (msg == "state")
    {
        QVariant var;
        stream >> var;
        SStateSensors state;
        state.byte = var.toInt();
        updateStateSensors(state);
    }
    else if (msg == "baseleds")
    {
        QVariant var;
        stream >> var;
        int leds = var.toInt();
        for (int i=0; i<6; ++i)
            mainLEDsBox[i]->setChecked(leds & (1<<i));
    }
    else if (msg == "m32leds")
    {
        QVariant var;
        stream >> var;
        int leds = var.toInt();

        for (int i=0; i<4; ++i)
            m32LEDsBox[i]->setChecked(leds & (1<<i));
    }
    else if (msg == "mic")
    {
        QVariant var;
        stream >> var;
        micPlot->addData("Microphone", var.toInt());
    }
    else if (msg == "destspeedleft")
    {
        QVariant var;
        stream >> var;
        motorSpeedPlot->addData("Left (destination)", var.toInt());
    }
    else if (msg == "destspeedright")
    {
        QVariant var;
        stream >> var;
        motorSpeedPlot->addData("Right (destination)", var.toInt());
    }
    else if (msg == "distleft")
    {
        QVariant var;
        stream >> var;
        motorDistanceLCD[0]->display(var.toInt());
        motorDistancePlot->addData("Left", var.toInt());
        motorDistance[0] = var.toInt();
    }
    else if (msg == "distright")
    {
        QVariant var;
        stream >> var;
        motorDistanceLCD[1]->display(var.toInt());
        motorDistancePlot->addData("Right", var.toInt());
        motorDistance[1] = var.toInt();
    }
    else if (msg == "destdistleft")
    {
        QVariant var;
        stream >> var;
        destMotorDistance[0] = var.toInt();
    }
    else if (msg == "destdistright")
    {
        QVariant var;
        stream >> var;
        destMotorDistance[1] = var.toInt();
    }
    else if (msg == "motordir")
    {
        QVariant var;
        stream >> var;
        SMotorDirections dir;
        dir.byte = var.toInt();
        updateMotorDirections(dir);
    }
    else if (msg == "rc5")
    {
        QVariant var;
        stream >> var;
        RC5data_t rc5;
        rc5.data = var.toInt();
        RC5DeviceLabel->setText(QString::number(rc5.device));
        RC5KeyLabel->setText(QString::number(rc5.key_code));
        RC5ToggleBitBox->setChecked(rc5.toggle_bit);
    }
    else
    {
        // Other data that needs to be averaged
        QVariant var;
        stream >> var;
        sensorDataMap[msg].total += var.toUInt();
        sensorDataMap[msg].count++;
    }
}

void CQtClient::updateStateSensors(const SStateSensors &state)
{
    bumperBox[0]->setChecked(state.bumperLeft);
    bumperBox[1]->setChecked(state.bumperRight);
    
    ACSCollisionBox[0]->setChecked(state.ACSLeft);
    ACSCollisionBox[1]->setChecked(state.ACSRight);
    
    ACSPlot->addData("Left", state.ACSLeft * ACSPowerSlider->value());
    ACSPlot->addData("Right", state.ACSRight * ACSPowerSlider->value());
    
    if (currentStateSensors.movementComplete && !state.movementComplete)
        appendLogText("Started movement.");
    else if (!currentStateSensors.movementComplete && state.movementComplete)
        appendLogText("Finished movement.");
    
    if (isScanning)
        updateScan(currentStateSensors, state);
    
    currentStateSensors.byte = state.byte;
    firstStateUpdate = false;
}

void CQtClient::updateScan(const SStateSensors &oldstate, const SStateSensors &newstate)
{
    appendLogText("updateScan()\n");
    
    // UNDONE: Which sensor?
    if (newstate.ACSLeft || newstate.ACSRight)
        scannerWidget->addPoint(motorDistance[0] * 360 / destMotorDistance[0],
                                ACSPowerState);
    else
        scannerWidget->endPoint();
    
    if (!firstStateUpdate && !oldstate.movementComplete && newstate.movementComplete)
    {
        stopScan();
        return;
    }

    if (alternatingScan && (ACSPowerState != ACS_POWER_OFF))
    {
        if (!remainingACSCycles && (newstate.ACSState < 2))
        {
            appendLogText("ACS Cycle reached\n");
            
            // UNDONE: Only change to more power when no hit
            EACSPowerState newpower = ACSPowerState;
            if (newpower == ACS_POWER_LOW)
                newpower = ACS_POWER_MED;
            else if (newpower == ACS_POWER_MED)
                newpower = ACS_POWER_HIGH;
            else // if (newacs == ACS_POWER_HIGH)
                newpower = ACS_POWER_LOW;
            
            remainingACSCycles = 2; // Wait two more cycles before next switch
            
            executeCommand(QString("set acs %1").arg(ACSPowerToString(newpower)));
        }
        
        if (remainingACSCycles && (oldstate.ACSState != ACS_STATE_WAIT_RIGHT) &&
            (newstate.ACSState == ACS_STATE_WAIT_RIGHT))
            --remainingACSCycles; // Reached another state cycle
    }
}

void CQtClient::updateMotorDirections(const SMotorDirections &dir)
{
    if (dir.byte != currentMotorDirections.byte)
    {
        QString leftS(directionString(dir.left)), rightS(directionString(dir.right));
        if (dir.left != currentMotorDirections.left)
            appendLogText(QString("Changed left motor direction to %1.\n").arg(leftS));
        
        if (dir.right != currentMotorDirections.right)
            appendLogText(QString("Changed right motor direction to %1.\n").arg(rightS));
        
        if (dir.destLeft != currentMotorDirections.destLeft)
            appendLogText(QString("Changed left destination motor direction to %1.\n").arg(directionString(dir.destLeft)));
        
        if (dir.destRight != currentMotorDirections.destRight)
            appendLogText(QString("Changed right destination motor direction to %1.\n").arg(directionString(dir.destRight)));
        
        currentMotorDirections.byte = dir.byte;
        motorDirection[0]->setText(leftS);
        motorDirection[1]->setText(rightS);
    }
}

void CQtClient::executeCommand(const QString &cmd)
{
    appendLogText(QString("Executing RP6 console command: \"%1\"\n").arg(cmd));
    
    if (clientSocket->state() == QAbstractSocket::ConnectedState)
    {
        CTcpWriter tcpWriter(clientSocket);
        tcpWriter << QString("command");
        tcpWriter << cmd;
        tcpWriter.write();
    }
    else
        appendLogText("WARNING: Cannot execute command: not connected!\n");
}

void CQtClient::changeDriveSpeedVar(int &speed, int delta)
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

void CQtClient::connectedToServer()
{
    appendLogText(QString("Connected to server (%1:%2)\n").
            arg(clientSocket->localAddress().toString()).
            arg(clientSocket->localPort()));
    firstStateUpdate = true;
}

void CQtClient::serverHasData()
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

void CQtClient::socketError(QAbstractSocket::SocketError error)
{
    QMessageBox::critical(this, "Socket error", clientSocket->errorString());
}

void CQtClient::updateSensors()
{
    for (QMap<QString, SSensorData>::iterator it=sensorDataMap.begin();
         it!=sensorDataMap.end(); ++it)
    {
        if (it.value().count == 0)
            continue;
        
        int data = it.value().total / it.value().count;
        
        if (it.key() == "lightleft")
        {
            lightSensorsLCD[0]->display(data);
            lightSensorsPlot->addData("Left", data);
        }
        else if (it.key() == "lightright")
        {
            lightSensorsLCD[1]->display(data);
            lightSensorsPlot->addData("Right", data);
        }
        else if (it.key() == "speedleft")
        {
            motorSpeedLCD[0]->display(data);
            motorSpeedPlot->addData("Left (actual)", data);
        }
        else if (it.key() == "speedright")
        {
            motorSpeedLCD[1]->display(data);
            motorSpeedPlot->addData("Right (actual)", data);
        }
        else if (it.key() == "motorcurrentleft")
        {
            motorCurrentLCD[0]->display(data);
            motorCurrentPlot->addData("Left", data);
        }
        else if (it.key() == "motorcurrentright")
        {
            motorCurrentLCD[1]->display(data);
            motorCurrentPlot->addData("Right", data);
        }
        else if (it.key() == "battery")
        {
            batteryLCD->display(data);
            batteryPlot->addData("Battery", data);
        }
        else if (it.key() == "acspower")
        {
            ACSPowerSlider->setValue(data);
            ACSPowerState = static_cast<EACSPowerState>(data);
        }
        
        it.value().total = it.value().count = 0;
    }
}

void CQtClient::connectToServer()
{
    clientSocket->abort();
    clientSocket->connectToHost(serverEdit->text(), 40000);
}

void CQtClient::setMicUpdateTime(int value)
{
    if (micUpdateToggleButton->isChecked())
        executeCommand(QString("set slavemic %1").arg(value));
}

void CQtClient::micPlotToggled(bool checked)
{
    if (checked)
        executeCommand(QString("set slavemic %1").arg(micUpdateTimeSpinBox->value()));
    else
        executeCommand("set slavemic 0");
}

void CQtClient::driveButtonPressed(int dir)
{
    const int speedchange = 5, startspeed = 60;
    
    if (currentStateSensors.bumperLeft || currentStateSensors.bumperRight || !currentStateSensors.movementComplete)
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
        
        driveSpeedSlider[0]->setValue((driveForwardSpeed < 0) ? -left : left);
        driveSpeedSlider[1]->setValue((driveForwardSpeed < 0) ? -right : right);
    }
    else
    {
        executeCommand(QString("set speed %1 %1").arg(abs(driveForwardSpeed)));
        driveSpeedSlider[0]->setValue(driveForwardSpeed);
        driveSpeedSlider[1]->setValue(driveForwardSpeed);
    }
}

void CQtClient::stopDriveButtonPressed()
{
    executeCommand("stop");
    driveForwardSpeed = 0;
    driveTurnSpeed = 0;
    driveTurning = false;
    driveSpeedSlider[0]->setValue(0);
    driveSpeedSlider[1]->setValue(0);
}

void CQtClient::startScan()
{
    isScanning = true;
    scannerWidget->clear();
    scanButton->setEnabled(false);
    
    if (scanPowerComboBox->currentText() == "Alternating")
    {
        appendLogText("Alternating scan started.\n");
        executeCommand("set acs low");
        remainingACSCycles = 2;
        alternatingScan = true;
    }
    else
    {
        alternatingScan = false;
        
        if (scanPowerComboBox->currentText() == "Low")
            executeCommand("set acs low");
        else if (scanPowerComboBox->currentText() == "Medium")
            executeCommand("set acs med");
        else if (scanPowerComboBox->currentText() == "High")
            executeCommand("set acs high");
    }
    
    executeCommand(QString("rotate 360 %1").arg(scanSpeedSpinBox->value()));
}

void CQtClient::stopScan()
{
    isScanning = false;
    scanButton->setEnabled(true);
}

void CQtClient::sendConsoleCommand()
{
    // Append cmd
    appendConsoleText(QString("> %1\n").arg(consoleIn->text()));
    executeCommand(consoleIn->text());
    consoleIn->clear();
}
