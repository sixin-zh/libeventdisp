#include "unit_task.h"

namespace nyu_libeventdisp {
const TaskGroupID UnitTask::DEFAULT_ID = 0;

UnitTask::UnitTask(const std::tr1::function<void ()> &task)
    : id(DEFAULT_ID), task(task) {
}

UnitTask::UnitTask(const std::tr1::function<void ()> &task, TaskGroupID id)
    : id(id), task(task) {
}
} //namespace

