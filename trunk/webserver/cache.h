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
#include <cstring>
#include <string>
#include <list>
#include <tr1/unordered_map>
#include <tr1/functional>

#include <boost/functional/hash.hpp>

#include "fast_fifo.h"

namespace nyu_libedisp_webserver {
union IntChar {
  uint32_t intVal;

  struct {
    char val1;
    char val2;
    char val3;
    char val4;
  } charVal;
};

// A simple implementation for computing hash of the contents of a char *.
// Note: only the first MAX_LEN_TO_HASH characters are computed into the hash.
struct HashCharPtr {
  static const size_t MAX_LEN_TO_HASH = 16;
  static const char PADDING = 0;
  static const size_t MODULUS_MASK = 0x3;
  
  size_t operator()(const char *c) const {
#ifndef USE_STRING_HASHING
    const size_t len = strlen(c);
    size_t charsHashed = 0;
    size_t hashSeed = 0;
    IntChar val;
    
    while (charsHashed < MAX_LEN_TO_HASH && charsHashed < len) {
      size_t toCopy = (len - charsHashed) & MODULUS_MASK;

      switch (toCopy) {
        case 0:
          // This is actully 4, actual value of toCopy before masked cannot
          // be 0, since it will never pass the condition charsHashed < len
          val.charVal.val1 = c[charsHashed++];
          val.charVal.val2 = c[charsHashed++];
          val.charVal.val3 = c[charsHashed++];
          val.charVal.val4 = c[charsHashed++];
          break;

        case 1:
          val.charVal.val1 = c[charsHashed++];
          val.charVal.val2 = PADDING;
          val.charVal.val3 = PADDING;
          val.charVal.val4 = PADDING;
          break;

        case 2:
          val.charVal.val1 = c[charsHashed++];
          val.charVal.val2 = c[charsHashed++];
          val.charVal.val3 = PADDING;
          val.charVal.val4 = PADDING;
          break;

        case 3:
          val.charVal.val1 = c[charsHashed++];
          val.charVal.val2 = c[charsHashed++];
          val.charVal.val3 = c[charsHashed++];
          val.charVal.val4 = PADDING;
          break;
      }

      boost::hash_combine(hashSeed, val.intVal);
    }

    return hashSeed;
#else
    std::tr1::hash<std::string> hasher;
    return hasher(std::string(c));
#endif
  }
};

// A simple functional object for testing string equality
struct EqCharPtr {
  size_t operator()(const char *lhs, const char *rhs) const {
    return (static_cast<size_t>(strcmp(lhs, rhs)) == 0);
  }
};

struct CacheData {
  const char *data;
  const size_t size;
  const bool isCached;

  CacheData(const char *data, size_t size, bool isCached);
};

typedef std::tr1::function<void (const CacheData &)> CacheCallback;

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

  // Puts an item into the cache and sets the reference count to 1. Also calls
  // all callbacks queued for this queue if it was previously reserved with
  // isCached set to true.
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
  //  callback - the callback object that will be called/queued when the
  //    cache has the data. The ownership of the callback will be passed to
  //    this object.
  //
  //    The following will be set to CacheData when the callback is called:
  //
  //    buf - will contain the object if successful. The buf will be guaranteed
  //      to be valid until and in the cache until doneWith is called or until
  //      this cache is destroyed.
  //    size - will contain the size of the object if successful.
  //    isCached - will be set to true when data was actually acquired from
  //      the cache.
  //
  // Returns false if no such key exists. Returns true if the object was
  //   successfully extracted from the cache or callback was successfully
  //   queued.
  bool get(const char *key, CacheCallback *callback);

  // Reserves a slot in the cache. This is an indication of intent to put data
  // for this key. Subsequent calls on get will have their callbacks queued and
  // will be triggered after a put or cancelReservation is called.
  //
  // Params:
  //  key - the key of the slot to reserve
  //
  // Returns true if reservation was successful. Fails if the slot is currently
  //   occupied or already reserved.
  bool reserve(const char *key);

  // Cancels a previous reservationt in the slot. Also calls all the callbacks
  // queued for this key with isCached set to false.
  //
  // Params:
  //  key - the key to cancel the reservation.
  //
  // Returns false if the key was not reserved.
  bool cancelReservation(const char *key);
  
  // Informs this cache object that the given object can be released. This has a
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
    size_t size;
    
    size_t refCount;
    FastFIFONode<CacheEntry> *freeListRef;
    bool isReserved; // slot is reserved, but data is still invalid
    std::list<CacheCallback *> queue;

    // Creates a reserved cache entry with no initial data
    CacheEntry(const char *name);

    // Creates a new cache entry
    CacheEntry(const char *name, const char *data, size_t size);

    // Deallocates the name & data field of this object.
    static void deallocate(CacheEntry &entry);
  };

  typedef std::tr1::unordered_map<const char *, CacheEntry,
                                  HashCharPtr, EqCharPtr> CacheMap;
  
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

