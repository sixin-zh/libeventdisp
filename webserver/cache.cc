#include "cache.h"
#include <cassert>
#include <cstdlib>
#include <map>

using std::make_pair;
using std::pair;
using std::list;

namespace nyu_libedisp_webserver {
CacheData::CacheData(const char *data, size_t size, bool isCached) :
    data(data), size(size), isCached(isCached) {
}

Cache::CacheEntry::CacheEntry(const char *name) :
    name(name), data(NULL), size(0), refCount(1),
    freeListRef(NULL), isReserved(true) {
}

Cache::CacheEntry::CacheEntry(const char *name, const char *data, size_t size) :
    name(name), data(data), size(size), refCount(1),
    freeListRef(NULL), isReserved(false) {
}

Cache::Cache(size_t sizeQuota) : sizeQuota_(sizeQuota), currentSize_(0) {
}

Cache::~Cache() {
  for (CacheMap::iterator iter = cacheMap_.begin();
       iter != cacheMap_.end(); ++iter) {
    CacheEntry &entry = iter->second;
    free(const_cast<char *>(entry.data));

    FastFIFONode<CacheEntry> *node = entry.freeListRef;
    if (node != NULL) {
      freeList_.removeNode(node);
    }
  }

  CacheEntry *entry;
  while (freeList_.pop(&entry)) {
    free(const_cast<char *>(entry->data));
  }
}

bool Cache::put(const char *key, const char *buf, size_t size) {
  bool ret = false;
  pair<CacheMap::iterator, bool> result =
      cacheMap_.insert(make_pair(key, CacheEntry(key, buf, size)));

  if (result.second) {
    currentSize_ += size;
    ret = true;
  }
  else {
    CacheEntry &entry = result.first->second;

    if (entry.isReserved) {
      entry.data = buf;
      entry.size = size;
      
      for (list<CacheCallback *>::iterator iter = entry.queue.begin();
           iter != entry.queue.end(); ++iter) {
        entry.refCount++;
        
        CacheCallback *callback = *iter;
        // Note: Passing stack object, don't queue into dispatcher
        (*callback)(CacheData(buf, size, true));
        delete callback;
      }

      entry.isReserved = false;
      ret = true;
    }
  }
  
  return ret;
}

bool Cache::get(const char *key, CacheCallback *callback) {
  bool ret = false;

  CacheMap::iterator iter = cacheMap_.find(key);

  if (iter != cacheMap_.end()) {
    CacheEntry &entry = iter->second;

    if (entry.isReserved && callback != NULL) {
      entry.queue.push_back(callback);
    }
    else {
      // Note: Passing stack object, don't queue into dispatcher
      (*callback)(CacheData(entry.data, entry.size, true));
      entry.refCount++;
    
      if (entry.freeListRef != NULL) {
        freeList_.removeNode(entry.freeListRef);
        entry.freeListRef = NULL;
      }

      delete callback;
    }

    ret = true;
  }
  else {
    delete callback;
  }
  
  return ret;
}

bool Cache::reserve(const char *key) {
  bool ret = false;
  pair<CacheMap::iterator, bool> result =
      cacheMap_.insert(make_pair(key, CacheEntry(key)));

  if (result.second) {
    ret = true;
  }
  else {
    CacheEntry &entry = result.first->second;
    
    if (!entry.isReserved) {
      entry.isReserved = true;
      ret = true;
    }
  }

  return ret;
}

bool Cache::cancelReservation(const char *key) {
  bool ret = false;
  CacheMap::iterator iter = cacheMap_.find(key);

  if (iter != cacheMap_.end()) {
    CacheEntry &entry = iter->second;
    
    if (entry.isReserved) {
      entry.isReserved = false;

      for (list<CacheCallback *>::iterator iter = entry.queue.begin();
           iter != entry.queue.end(); ++iter) {
        CacheCallback *callback = *iter;
        // Note: Passing stack object, don't queue into dispatcher
        (*callback)(CacheData(NULL, 0, false));
        delete callback;
      }
      
      ret = true;
    }
  }

  return ret;
}

void Cache::doneWith(const char *key) {
  CacheMap::iterator iter = cacheMap_.find(key);

  if (iter != cacheMap_.end()) {
    CacheEntry *entry = &iter->second;

    assert(entry->refCount > 0);
    if (--entry->refCount == 0) {
      entry->freeListRef = freeList_.push(entry);
    }
  }
  else {
    assert(false);
  }

  if (currentSize_ > sizeQuota_) {
    cleanup(currentSize_ - sizeQuota_);
  }
}

void Cache::cleanup(size_t space) {
  size_t spaceFreed = 0;
  CacheEntry *entry = NULL;

  // Try to delete data from the free list
  while (freeList_.pop(&entry) && spaceFreed < space) {
    assert(entry->refCount == 0);
    free(const_cast<char *>(entry->data));
    spaceFreed += entry->size;
    cacheMap_.erase(entry->name);
  }

  currentSize_ -= spaceFreed;
}
}
