#ifndef CLOCK_H
#define CLOCK_H

// For Q_OS_WIN
#include <QtGlobal>

#ifdef Q_OS_WIN

#include <time.h>

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};

#define CLOCK_MONOTONIC 0

int clock_gettime(int X, struct timespec *tv);

#endif

#endif // CLOCK_H
