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

#ifndef LIBEVENTDISP_TEST_HELPER_H_
#define LIBEVENTDISP_TEST_HELPER_H_

#include <cstddef>
#include "unit_task.h"

namespace nyu_libeventdisp {
class EventDispatcherInterface;
}

namespace nyu_libeventdisp_test {
void delayedInc(nyu_libeventdisp::EventDispatcherInterface *dispatcher,
                size_t *val, int delay, nyu_libeventdisp::TaskGroupID id);
}

#endif

