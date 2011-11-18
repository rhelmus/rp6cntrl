#ifndef PLUGINTHREAD_H
#define PLUGINTHREAD_H

#include "simulator.h"

#include <QThread>

class CCallPluginMainThread : public QThread
{
    TCallPluginMainFunc mainFunc;
    Qt::HANDLE threadID;

public:
    CCallPluginMainThread(TCallPluginMainFunc func, QObject *parent = 0);

#ifndef Q_OS_WIN
    pthread_t getThreadID(void) const { return (pthread_t)threadID; }
#endif

public slots:
    void run(void);
};

#endif // PLUGINTHREAD_H
