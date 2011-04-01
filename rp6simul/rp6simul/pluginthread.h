#ifndef PLUGINTHREAD_H
#define PLUGINTHREAD_H

#include "rp6simul.h"

#include <QThread>

class CCallPluginMainThread : public QThread
{
    TCallPluginMainFunc mainFunc;

public:
    CCallPluginMainThread(TCallPluginMainFunc func, QObject *parent = 0);

public slots:
    void run(void);
};

#endif // PLUGINTHREAD_H
