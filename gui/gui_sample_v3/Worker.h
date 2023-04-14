#ifndef WORKER_H
#define WORKER_H

#include <QThread>

typedef void (*tWorkerFunc)(void*);

class Worker : public QThread
{
    Q_OBJECT
    void run() override;

public:
    void* param;
    tWorkerFunc func;

public:
    void setFunc(tWorkerFunc f, void* p);
};

#endif // WORKER_H
