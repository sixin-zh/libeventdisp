#include "cache.h"

namespace nyu_libedisp_webserver {
Cache::Cache(size_t sizeQuota) : sizeQuota_(sizeQuota) {
}

bool Cache::put(const char *key, char *buf, size_t size) {
  return false;
}

bool Cache::get(const char *key, char **buff, size_t &size) {
  return false;
}

void Cache::doneWith(const char *key) {
}
}

