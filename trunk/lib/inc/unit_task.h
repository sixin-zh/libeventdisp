// Copyright 2011 libeventdisp
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIBEVENTDISP_UNIT_TASK_H_
#define LIBEVENTDISP_UNIT_TASK_H_

#include <tr1/functional>

namespace nyu_libeventdisp {
typedef unsigned long TaskGroupID;

struct UnitTask {
  static const TaskGroupID DEFAULT_ID;
  const TaskGroupID id;
  std::tr1::function<void ()> task;

  UnitTask(const std::tr1::function<void ()> &task);
  UnitTask(const std::tr1::function<void ()> &task, TaskGroupID id);
};
}

#endif

