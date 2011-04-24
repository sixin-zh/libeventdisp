#include "cache.h"
#include <cassert>
#include <cstdlib>
#include <map>

using std::make_pair;

namespace nyu_libedisp_webserver {
Cache::CacheEntry::CacheEntry(const char *name, const char *data, size_t size) :
    name(name), data(data), size(size), refCount(1), freeListRef(NULL) {
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

  CacheMap::iterator iter = cacheMap_.find(key);

  if (iter == cacheMap_.end()) {
    cacheMap_.insert(make_pair(key, CacheEntry(key, buf, size)));
    currentSize_ += size;
    ret = true;
  }
  
  return ret;
}

bool Cache::get(const char *key, const char **buf, size_t &size) {
  bool ret = false;

  CacheMap::iterator iter = cacheMap_.find(key);

  if (iter != cacheMap_.end()) {
    CacheEntry &entry = iter->second;
    *buf = entry.data;
    size = entry.size;
    entry.refCount++;
    
    if (entry.freeListRef != NULL) {
      freeList_.removeNode(entry.freeListRef);
      entry.freeListRef = NULL;
    }

    ret = true;
  }
  
  return ret;
}

void Cache::doneWith(const char *key) {
  CacheMap::iterator iter = cacheMap_.find(key);

  if (iter != cacheMap_.end()) {
    CacheEntry *entry = &iter->second;

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

