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
#include <tr1/unordered_map>

#include "fast_fifo.h"

namespace nyu_libedisp_webserver {
// A simple key-value cache with a fixed quota that also keeps count of how many
// clients are using a cached object. This implementation is not thread-safe.
class Cache {
 public:
  // Creates a new cache instance.
  //
  // Param:
  //  sizeQuota - the maximum size of data this cache should hold in bytes.
  explicit Cache(size_t sizeQuota);
  ~Cache();

  // Puts an item into the cache and sets the reference count to 1.
  //
  // Params:
  //  key - the name for the object to put into this cache.
  //  buf - the object to put into the cache. The ownership of buf will be
  //    transferred to this cache when this operation was successful.
  //    Important: buf should be allocated using malloc and not new.
  //  size - the size of the object in bytes.
  //
  // Returns true if buf was successfully stored into this cache. This can
  //   fail if there is already an existing entry with the same name as key.
  bool put(const char *key, const char *buf, size_t size);

  // Gets an item from the cache. This also has a side effect of removing the
  // object from the free list and incrementing the reference count for the
  // object.
  //
  // Params:
  //  key - the name of the object to extract from the cache.
  //  buf - will contain the object if successful. The buf will be guaranteed
  //    to be valid until and in the cache until doneWith is called or until
  //    this cache is destroyed.
  //  size - will contain the size of the object if successful.
  //
  // Returns true if the object was successfully extracted from the cache.
  bool get(const char *key, const char **buf, size_t &size);

  // Inform this cache object that the given object can be released. This has a
  // side effect of decrementing the reference counter for the object and also
  // attempts to free objects from the free list if the quota is reached.
  //
  // Param:
  //  key - the name of the object.
  void doneWith(const char *key);

 private:
  struct CacheEntry {
    const char *name;
    const char *data;
    const size_t size;
    
    size_t refCount;
    FastFIFONode<CacheEntry> *freeListRef;

    CacheEntry(const char *name, const char *data, size_t size);
  };

  typedef std::tr1::unordered_map<const char *, CacheEntry> CacheMap;
  
  const size_t sizeQuota_;
  size_t currentSize_;
  CacheMap cacheMap_;
  FastFIFO<CacheEntry> freeList_;

  // Cleans up the free list.
  //
  // Param:
  //  space - the minimum size to clean up. Note that this is best effort and
  //    is not guaranteed.
  void cleanup(size_t space);
  
  // Disallow copying
  Cache(const Cache&);
  void operator=(const Cache&);
};
}

#endif

