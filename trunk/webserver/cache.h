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

#ifndef LIBEVENTDISP_CACHE_H_
#define LIBEVENTDISP_CACHE_H_

#include <cstddef>

namespace nyu_libedisp_webserver {
// A simple key-value cache with a fixed quota that also keeps count of how many
// clients are using a cached object.
class Cache {
 public:
  // Create a new cache instance.
  //
  // Param:
  //  sizeQuota - the maximum size of data this cache should hold in bytes.
  explicit Cache(size_t sizeQuota);

  // Puts an item in the cache and set the reference count to 1. This also
  // attempts to free objects from the free list if the quota is reached.
  //
  // Params:
  //  key - the name for the object to put into the cache.
  //  buf - the object to put into the cache. The object life cycle will be
  //    managed by this cache only when it is successfully stored into the
  //    cache.
  //  size - the size of the ojbject in bytes.
  //
  // Returns true if buf was successfully stored into the cache. Also returns
  //   false if the cache already contains an object with the same name as key.
  bool put(const char *key, char *buf, size_t size);

  // Gets an item from the cache. This also has a side effect of removing the
  // object from the free list and incrementing the reference count for the
  // object.
  //
  // Params:
  //  key - the name of the object to extract from the cache.
  //  buf - will contain the object if successful. The buf will be guaranteed
  //    to be valid until 
  //  size - will contain the size of the object if successful.
  //
  // Returns true if the object was successfully extracted from the cache.
  bool get(const char *key, char **buff, size_t &size);

  // Completely remove an object from the cache. The object will be gone forever
  // and will not be moved to the free list.
  // void remove(const char *key);

  // Inform this cache object that the given object can be released. This has a
  // side effect of decrementing the reference counter for the object.
  void doneWith(const char *key);

 private:
  size_t sizeQuota_;
};
}

#endif

