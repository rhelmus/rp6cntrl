#include "rp6simul.h"
#include "pluginthread.h"
#include "iohandler.h"
#include "avrtimer.h"

#include <QtGui>
#include <QLibrary>

namespace {

template <typename TF> TF getLibFunc(QLibrary &lib, const char *f)
{
    TF ret = (TF)lib.resolve(f);

    if (!ret)
        qDebug() << "Failed to get lib function" << f;

    return ret;
}

QString constructISRFunc(EISRTypes type)
{
    QString f;

    switch (type)
    {
    case ISR_USART_RXC_vect: f = "USART_RXC_vect"; break;
    case ISR_TIMER0_COMP_vect: f = "TIMER0_COMP_vect"; break;
    case ISR_TIMER1_COMPA_vect: f = "TIMER1_COMPA_vect"; break;
    case ISR_TIMER2_COMP_vect: f = "TIMER2_COMP_vect"; break;
    default: Q_ASSERT(false); break;
    }

    return "ISR_" + f;
}

const char *getCString(const QString &s)
{
    static QByteArray by;
    by = s.toLatin1();
    return by.data();
}

}

CRP6Simulator *CRP6Simulator::instance = 0;
QReadWriteLock CRP6Simulator::generalIOReadWriteLock;

CRP6Simulator::CRP6Simulator(QWidget *parent) : QMainWindow(parent),
    pluginMainThread(0)
{
    Q_ASSERT(!instance);
    instance = this;

    QWidget *cw = new QWidget(this);
    setCentralWidget(cw);

    QVBoxLayout *vbox = new QVBoxLayout(cw);

    QPushButton *button = new QPushButton("Start");
    connect(button, SIGNAL(clicked()), this, SLOT(runPlugin()));
    vbox->addWidget(button);

    initAVRClock();
    initIOHandlers();
    initPlugin();
}

CRP6Simulator::~CRP6Simulator()
{
    terminateAVRClock();
    delete AVRClock;
    terminatePluginMainThread();
}

void CRP6Simulator::initAVRClock()
{
    // From http://labs.qt.nokia.com/2010/06/17/youre-doing-it-wrong/

    AVRClock = new CAVRClock;
    AVRClockThread = new QThread(this);

    // UNDONE
//    connect(AVRClockThread, SIGNAL(started()), AVRClock, SLOT(start()));
    connect(AVRClockThread, SIGNAL(finished()), AVRClock, SLOT(stop()));

    AVRClock->moveToThread(AVRClockThread);

    AVRClockThread->start();
}

void CRP6Simulator::addIOHandler(CBaseIOHandler *handler)
{
    IOHandlerList << handler;
    handler->registerHandler(IOHandlerArray);
}

void CRP6Simulator::initIOHandlers()
{
    addIOHandler(new CUARTHandler(this));
    addIOHandler(new CTimer0Handler(this));
    addIOHandler(new CTimerMaskHandler(this));
}

void CRP6Simulator::terminateAVRClock()
{
    qDebug() << "Terminating AVR clock thread";
    AVRClockThread->quit();
    AVRClockThread->wait();
}

void CRP6Simulator::terminatePluginMainThread()
{
    if (pluginMainThread)
    {
        qDebug() << "Terminating plugin main thread";
        pluginMainThread->terminate();

        // UNDONE: Use terminated slot
        // UNDONE: Thread does not always exit
        if (!pluginMainThread->wait(250))
            qDebug() << "Failed to terminate plugin main thread!";

        delete pluginMainThread;
        pluginMainThread = 0;
    }
}

void CRP6Simulator::initPlugin()
{
    // UNDONE: unload previous library (? probably only name is cached)

    QLibrary lib("../avrglue/libmyrp6.so"); // UNDONE

    if (!lib.load())
    {
        QMessageBox::critical(this, "Error",
                              "Failed to load library:\n" + lib.errorString());
        return;
    }

    TCallPluginMainFunc mainfunc =
            getLibFunc<TCallPluginMainFunc>(lib, "callAvrMain");
    TSetGeneralIOCallbacks setiocb =
            getLibFunc<TSetGeneralIOCallbacks>(lib, "setGeneralIOCallbacks");

    if (!mainfunc || !setiocb)
        return; // UNDONE

    AVRClock->stop();

    terminatePluginMainThread();

    pluginMainThread = new CCallPluginMainThread(mainfunc, this);

    setiocb(generalIOSetCB, generalIOGetCB);

    for (int i=0; i<IO_END; ++i)
        generalIOData[i] = 0;

    for (int i=0; i<ISR_END; ++i)
    {
        ISRCacheArray[i] = 0;
        ISRFailedArray[i] = false;
    }

    foreach (CBaseIOHandler *handler, IOHandlerList)
    {
        handler->initPlugin();
    }
}

void CRP6Simulator::generalIOSetCB(EGeneralIOTypes type, TGeneralIOData data)
{
    instance->setGeneralIO(type, data);
    if (instance->IOHandlerArray[type])
        instance->IOHandlerArray[type]->handleIOData(type, data);
}

TGeneralIOData CRP6Simulator::generalIOGetCB(EGeneralIOTypes type)
{
    QReadLocker locker(&generalIOReadWriteLock);
    return instance->generalIOData[type];
}

void CRP6Simulator::runPlugin()
{
    if (pluginMainThread && !pluginMainThread->isRunning())
    {
        // NOTE: AVR clock should be stopped earlier
        AVRClock->reset();
        AVRClock->start();
        pluginMainThread->start();
    }
}

void CRP6Simulator::setGeneralIO(EGeneralIOTypes type, TGeneralIOData data)
{
    QWriteLocker locker(&generalIOReadWriteLock);
    generalIOData[type] = data;
}

void CRP6Simulator::execISR(EISRTypes type)
{
    // UNDONE: disable ISR calling

    if (ISRFailedArray[type])
        return;

    if (!ISRCacheArray[type])
    {
        QLibrary lib("../avrglue/libmyrp6.so"); // UNDONE
        QString func = constructISRFunc(type);
        ISRCacheArray[type] = getLibFunc<TISR>(lib, getCString(func));

        if (!ISRCacheArray[type])
        {
            ISRFailedArray[type] = true;
            return;
        }
    }

    ISRCacheArray[type]();
}
