#include "rp6simul.h"
#include "avrtimer.h"
#include "lua.h"
#include "projectwizard.h"
#include "robotgraphicsitem.h"
#include "simulator.h"
#include "utils.h"

#include <QtGui>

namespace {

void activateDockTab(QObject *p, const QString &tab)
{
    // Based on http://www.qtcentre.org/threads/21362-Setting-the-active-tab-with-tabified-docking-windows
    QList<QTabBar *> tabs = p->findChildren<QTabBar *>();
    foreach (QTabBar *t, tabs)
    {
        const int count = t->count();
        for (int i=0; i<count; ++i)
        {
            if (t->tabText(i) == tab)
            {
                t->setCurrentIndex(i);
                return;
            }
        }
    }

    qDebug() << "Warning: couldn't activate dock tab" << tab << "(not found)";
}

}

CRP6Simulator *CRP6Simulator::instance = 0;

CRP6Simulator::CRP6Simulator(QWidget *parent) : QMainWindow(parent)
{
    Q_ASSERT(!instance);
    instance = this;

    simulator = new CSimulator(this);
    connect(simulator->getAVRClock(), SIGNAL(clockSpeed(unsigned long)), this,
            SLOT(updateClockDisplay(unsigned long)));

    resize(850, 600);

    projectWizard = new CProjectWizard(this);

    createMenus();
    createToolbars();

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);

    splitter->addWidget(createMainWidget());
    splitter->addWidget(createLogWidgets());
    splitter->setSizes(QList<int>() << 600 << 300);

    QDockWidget *statdock, *regdock;
    addDockWidget(Qt::RightDockWidgetArea, statdock = createStatusDock(),
                  Qt::Vertical);
    addDockWidget(Qt::RightDockWidgetArea, regdock = createRegisterDock(),
                  Qt::Vertical);
    tabifyDockWidget(statdock, regdock);
    activateDockTab(this, "Status");

    pluginUpdateUITimer = new QTimer(this);
    connect(pluginUpdateUITimer, SIGNAL(timeout()), this, SLOT(timedUpdate()));
    pluginUpdateUITimer->setInterval(250);

    initLua();
}

void CRP6Simulator::createMenus()
{
    QMenu *menu = menuBar()->addMenu("&File");
    menu->addAction("New", this, SLOT(newProject()), tr("ctrl+N"));
    menu->addAction("Open", this, SLOT(openProject()), tr("ctrl+O"));
    menu->addMenu("Open recent...")->addAction("blah");
    menu->addSeparator();
    menu->addAction("Quit", this, SLOT(close()), tr("ctrl+Q"));

    menu = menuBar()->addMenu("&Edit");
    menu->addAction("Project settings");
    menu->addSeparator();
    menu->addAction("Preferences");

    menu = menuBar()->addMenu("&Help");
    menu->addAction("About");
    menu->addAction("About Qt", qApp, SLOT(aboutQt()));
}

void CRP6Simulator::createToolbars()
{
    QToolBar *toolb = addToolBar("Run");
    runPluginAction = toolb->addAction(QIcon(style()->standardIcon(QStyle::SP_MediaPlay)), "Run",
                                       this, SLOT(runPlugin()));
    runPluginAction->setShortcut(tr("ctrl+R"));
    runPluginAction->setEnabled(false);

    stopPluginAction = toolb->addAction(QIcon(style()->standardIcon(QStyle::SP_MediaStop)), "Run",
                                        this, SLOT(stopPlugin()));
    stopPluginAction->setShortcut(tr("esc"));
    stopPluginAction->setEnabled(false);
}

QWidget *CRP6Simulator::createMainWidget()
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    QRectF screct(-300.0, -300.0, 600.0, 600.0);
    scene->setSceneRect(screct);

    robotGraphicsItem = new CRobotGraphicsItem;
    robotGraphicsItem->setPos(0, 50);
    scene->addItem(robotGraphicsItem);

    // Obstacle
//    scene->addRect(-100, -100, 200, 50, QPen(), QBrush(Qt::red));
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap("../resource/wooden-block.png").scaled(200.0, 100.0));
    p->setPos(-100.0, -100.0);


    // Edges
    scene->addRect(screct.x(), screct.y(), screct.width(), 5, QPen(), QBrush(Qt::black));
    scene->addRect(screct.x(), screct.y(), 5, screct.height(), QPen(), QBrush(Qt::black));
    scene->addRect(screct.x(), screct.bottom()-5, screct.width(), 5, QPen(), QBrush(Qt::black));
    scene->addRect(screct.right()-5, screct.y(), 5, screct.height(), QPen(), QBrush(Qt::black));

    QWidget *ret = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    graphicsView = new QGraphicsView(scene);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setBackgroundBrush(QBrush(QPixmap("../resource/floor.jpg")));
    new QShortcut(QKeySequence("+"), this, SLOT(zoomSceneIn()));
    new QShortcut(QKeySequence("-"), this, SLOT(zoomSceneOut()));
    hbox->addWidget(graphicsView);

    QVBoxLayout *vbox = new QVBoxLayout;
    hbox->addLayout(vbox);

    QSpinBox *spinbox = new QSpinBox;
    connect(spinbox, SIGNAL(valueChanged(int)), this,
            SLOT(debugSetRobotLeftPower(int)));
    spinbox->setRange(-100, 100);
    new QShortcut(QKeySequence("Q"), spinbox, SLOT(stepDown()));
    new QShortcut(QKeySequence("W"), spinbox, SLOT(stepUp()));
    vbox->addWidget(spinbox);

    spinbox = new QSpinBox;
    connect(spinbox, SIGNAL(valueChanged(int)), this,
            SLOT(debugSetRobotRightPower(int)));
    spinbox->setRange(-100, 100);
    new QShortcut(QKeySequence("A"), spinbox, SLOT(stepDown()));
    new QShortcut(QKeySequence("S"), spinbox, SLOT(stepUp()));
    vbox->addWidget(spinbox);

    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), scene, SLOT(advance()));
    timer->start(1000 / 33);

    return ret;
}

QWidget *CRP6Simulator::createLogWidgets()
{
    QTabWidget *ret = new QTabWidget;
    ret->setTabPosition(QTabWidget::South);

    ret->addTab(logWidget = new QPlainTextEdit, "log");
    logWidget->setReadOnly(true);
    logWidget->setCenterOnScroll(true);
    connect(this, SIGNAL(logTextReady(const QString &)), logWidget,
            SLOT(appendHtml(const QString &)));

    QWidget *w = new QWidget(ret);
    ret->addTab(w, "serial");
    QGridLayout *grid = new QGridLayout(w);

    grid->addWidget(serialOutputWidget = new QPlainTextEdit, 0, 0, 1, 3);
    serialOutputWidget->setReadOnly(true);
    serialOutputWidget->setCenterOnScroll(true);

    grid->addWidget(serialInputWidget = new QLineEdit, 1, 0);

    grid->addWidget(serialSendButton = new QPushButton("Send"), 1, 1);
    serialSendButton->setEnabled(false);
    connect(serialSendButton, SIGNAL(clicked()), this, SLOT(sendSerialPressed()));
    connect(serialInputWidget, SIGNAL(returnPressed()), serialSendButton, SLOT(click()));

    QPushButton *button = new QPushButton("Clear");
    connect(button, SIGNAL(clicked()), serialOutputWidget, SLOT(clear()));
    grid->addWidget(button, 1, 2);

    return ret;
}

QDockWidget *CRP6Simulator::createStatusDock()
{
    QDockWidget *ret = new QDockWidget("Status", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    QGroupBox *group = new QGroupBox("Clock");
    vbox->addWidget(group);
    QFormLayout *form = new QFormLayout(group);

    form->addRow("RP6 clock (MHz)", clockDisplay = new QLCDNumber);

    group = new QGroupBox("Robot");
    vbox->addWidget(group);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    subvbox->addWidget(robotStatusTreeWidget = new QTreeWidget);
    robotStatusTreeWidget->setHeaderLabels(QStringList() << "Device" << "Value");
    robotStatusTreeWidget->setSortingEnabled(true);
    robotStatusTreeWidget->sortByColumn(0, Qt::AscendingOrder);

    group = new QGroupBox("Drivers");
    vbox->addWidget(group);
    subvbox = new QVBoxLayout(group);

    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderLabels(QStringList() << "Driver" << "Status");
    tree->setRootIsDecorated(false);
    QList<QTreeWidgetItem *> items;
    items << new QTreeWidgetItem(QStringList() << "leds") <<
             new QTreeWidgetItem(QStringList() << "timer0") <<
             new QTreeWidgetItem(QStringList() << "timer1") <<
             new QTreeWidgetItem(QStringList() << "timer2") <<
             new QTreeWidgetItem(QStringList() << "motor");
    tree->addTopLevelItems(items);
    subvbox->addWidget(tree);

    return ret;
}

QDockWidget *CRP6Simulator::createRegisterDock()
{
    extern const char *IORegisterStringArray[IO_END];

    QDockWidget *ret = new QDockWidget("Registers", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    vbox->addWidget(IORegisterTableWidget = new QTableWidget(IO_END, 3));
    IORegisterTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    IORegisterTableWidget->setAlternatingRowColors(true);
    IORegisterTableWidget->setHorizontalHeaderLabels(QStringList() << "DEC" << "HEX" << "BIN");
    QStringList rows;
    for (int i=0; i<IO_END; ++i)
    {
        rows << IORegisterStringArray[i];
        for (int j=0; j<3; ++j)
        {
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            IORegisterTableWidget->setItem(i, j, item);
        }
    }
    IORegisterTableWidget->setVerticalHeaderLabels(rows);

    return ret;
}

void CRP6Simulator::initLua()
{
    simulator->initLua();

    NLua::registerFunction(luaAppendLogOutput, "appendLogOutput");
    NLua::registerFunction(luaAppendSerialOutput, "appendSerialOutput");
    NLua::registerFunction(luaUpdateRobotStatus, "updateRobotStatus");

    NLua::luaInterface.exec();

    // UNDONE: Needed?
    lua_getglobal(NLua::luaInterface, "init");
    lua_call(NLua::luaInterface, 0, 0);
}

void CRP6Simulator::openProjectFile(const QString &file)
{
    if (!simulator->openProjectFile(file))
        return;
    stopPluginAction->setEnabled(false);
    runPluginAction->setEnabled(true);
}

QString CRP6Simulator::getLogOutput(ELogType type, QString text) const
{
    text = Qt::escape(text);
    // Html doesn't like tabs too much
    text = text.replace('\t', QString("&nbsp;").repeated(4));
    QString fs;
    switch (type)
    {
    case LOG_LOG: fs = text; break;
    case LOG_WARNING: fs = QString("<FONT color=#FF8040>WARNING:</FONT> %1").arg(text); break;
    case LOG_ERROR: fs = QString("<FONT color=#FF0000><strong>ERROR: </strong></FONT> %1").arg(text); break;
    }

    return QString("<FONT color=#0000FF><strong>[%1]</strong></FONT> %2")
            .arg(QTime::currentTime().toString()).arg(fs);
}

void CRP6Simulator::appendLogOutput(ELogType type, const QString &text)
{
    logWidget->appendHtml(getLogOutput(type, text));
}

void CRP6Simulator::scaleGraphicsView(qreal f)
{
    // From Qt's elastic nodes example
    qreal factor = graphicsView->transform().scale(f, f).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    graphicsView->scale(f, f);
}

int CRP6Simulator::luaAppendLogOutput(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    const char *type = luaL_checkstring(l, 1);
    const char *text = luaL_checkstring(l, 2);

    ELogType t;
    if (!strcmp(type, "LOG"))
        t = LOG_LOG;
    else if (!strcmp(type, "WARNING"))
        t = LOG_WARNING;
    else if (!strcmp(type, "ERROR"))
        t = LOG_ERROR;
    else
        luaL_argerror(l, 1, "Wrong log type.");

    // Emit: function called outside main thread
    instance->emit logTextReady(instance->getLogOutput(t, text));
    return 0;
}

int CRP6Simulator::luaAppendSerialOutput(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    QMutexLocker seriallocker(&instance->serialBufferMutex);
    instance->serialTextBuffer += luaL_checkstring(l, 1);
    return 0;
}

int CRP6Simulator::luaUpdateRobotStatus(lua_State *l)
{
    QMutexLocker statlocker(&instance->robotStatusMutex);
    NLua::CLuaLocker lualocker;
    const int nargs = lua_gettop(l);
    QStringList strtree;

    for (int i=1; i<=nargs; ++i)
        strtree << luaL_checkstring(l, i);

    instance->robotStatusUpdateBuffer.append(strtree);
    return 0;
}

void CRP6Simulator::updateClockDisplay(unsigned long hz)
{
    double mhz = static_cast<double>(hz) / 1000000.0;
    QString s = QString::number(mhz, 'g', 3);
    if ((s.length() < 3) && !s.contains('.'))
    {
        s += ".";
        s = s.leftJustified(4, '0');
    }

    clockDisplay->display(s);
}

void CRP6Simulator::newProject()
{
    // UNDONE: Check if running

    // According to docs this should be called auto when show is called,
    // but it's not ...
    projectWizard->restart();
    if (projectWizard->exec() == QDialog::Accepted)
        openProjectFile(projectWizard->getProjectFile());
}

void CRP6Simulator::openProject()
{
    QString file =
            QFileDialog::getOpenFileName(this, "Open project file",
                                         QDir::homePath(),
                                         "RP6 simulator project files (*.rp6)");
    // UNDONE: Check if running
    if (!file.isEmpty())
        openProjectFile(file);
}

void CRP6Simulator::timedUpdate()
{
    QReadLocker iolocker(&simulator->getIORegisterLock());
    const TIORegisterData *ioregisters = simulator->getIORegisterArray();

    for (int i=0; i<IO_END; ++i)
    {
        TIORegisterData d = ioregisters[i]; // Shortcut
        IORegisterTableWidget->item(i, 0)->setText(QString::number(d));
        IORegisterTableWidget->item(i, 1)->setText(QString::number(d, 16));
        IORegisterTableWidget->item(i, 2)->setText(QString::number(d, 2).rightJustified(16, '0', true));
        IORegisterTableWidget->resizeRowToContents(i);
    }

    IORegisterTableWidget->resizeColumnsToContents();

    QMutexLocker serialocker(&instance->serialBufferMutex);
    if (!serialTextBuffer.isEmpty())
    {
        // Append text, without adding a new paragraph
        QTextCursor cur = serialOutputWidget->textCursor();
        cur.movePosition(QTextCursor::End);
        serialOutputWidget->setTextCursor(cur);
        serialOutputWidget->insertPlainText(serialTextBuffer);
        serialTextBuffer.clear();
    }

    QMutexLocker statlocker(&robotStatusMutex);
    foreach (QStringList stritems, robotStatusUpdateBuffer)
    {
        QList<QTreeWidgetItem *> items =
                instance->robotStatusTreeWidget->findItems(stritems.first(), Qt::MatchFixedString | Qt::MatchCaseSensitive);

        QTreeWidgetItem *parent;

        if (!items.isEmpty())
        {
            Q_ASSERT(items.size() == 1);
            parent = items.first();
        }
        else
            parent = new QTreeWidgetItem(instance->robotStatusTreeWidget, QStringList() << stritems.first());

        const int size = stritems.size();
        for (int i=1;i<(size-1); ++i)
        {
            const int childcount = parent->childCount();
            QTreeWidgetItem *child = 0;

            for (int j=0; j<childcount; ++j)
            {
                if (parent->child(j)->text(0) == stritems[i])
                {
                    child = parent->child(j);
                    break;
                }
            }

            if (!child)
                child = new QTreeWidgetItem(parent, QStringList() << stritems[i]);

            parent = child;
        }

        parent->setText(1, stritems.last());
    }
    robotStatusUpdateBuffer.clear();
}

void CRP6Simulator::runPlugin()
{
    if (!simulator->runPlugin())
        return;

    serialOutputWidget->clear();
    robotStatusTreeWidget->clear();
    pluginUpdateUITimer->start();

    runPluginAction->setEnabled(false);
    stopPluginAction->setEnabled(true);
    serialSendButton->setEnabled(true);
}

void CRP6Simulator::stopPlugin()
{
    simulator->stopPlugin();

    pluginUpdateUITimer->stop();
    timedUpdate(); // Dump any remaining buffered data

    runPluginAction->setEnabled(true);
    stopPluginAction->setEnabled(false);
    serialSendButton->setEnabled(false);
}

void CRP6Simulator::zoomSceneIn()
{
    scaleGraphicsView(1.2);
}

void CRP6Simulator::zoomSceneOut()
{
    scaleGraphicsView(1/1.2);
}

void CRP6Simulator::sendSerialPressed()
{
    NLua::CLuaLocker lualocker;
    lua_getglobal(NLua::luaInterface, "sendSerial");
    lua_pushstring(NLua::luaInterface, getCString(serialInputWidget->text() + "\n"));
    lua_call(NLua::luaInterface, 1, 0);

    serialOutputWidget->appendPlainText(QString("> %1\n").arg(serialInputWidget->text()));

    serialInputWidget->clear();
}

void CRP6Simulator::debugSetRobotLeftPower(int power)
{
    robotGraphicsItem->setLeftMotor(power);
}

void CRP6Simulator::debugSetRobotRightPower(int power)
{
    robotGraphicsItem->setRightMotor(power);
}
