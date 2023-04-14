#include "Worker.h"

void Worker::run() {
    if (func) func(param);
}

void Worker::setFunc(tWorkerFunc f, void *p) {
    func = f; param = p;
}
