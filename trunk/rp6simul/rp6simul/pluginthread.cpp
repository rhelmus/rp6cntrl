#include "pluginthread.h"

CCallPluginMainThread::CCallPluginMainThread(TCallPluginMainFunc func,
                                             QObject *parent) :
    QThread(parent), mainFunc(func)
{
}

void CCallPluginMainThread::run()
{
    Q_ASSERT(mainFunc != 0);
    setTerminationEnabled(true);
    mainFunc();
}
