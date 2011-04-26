#include "gtest/gtest.h"
#include "cache.h"

#include <tr1/functional>

using nyu_libedisp_webserver::Cache;
using nyu_libedisp_webserver::CacheData;
using nyu_libedisp_webserver::CacheCallback;

using std::tr1::bind;
using namespace std::tr1::placeholders;

namespace {
void copyCacheData(const CacheData &cacheData, const char **buf, size_t *size) {
  if (cacheData.isCached) {
    *buf = cacheData.data;
    *size = cacheData.size;
  }
}

CacheCallback* newCB(const CacheCallback &cb) {
  return new CacheCallback(cb);
}

void checkIfDataCached(const CacheData &cacheData, bool *isCached) {
  *isCached = cacheData.isCached;
}
} // namespace

TEST(CacheTest, Empty) {
  Cache cache(10);
  
  ASSERT_FALSE(cache.get("c", NULL));
}

TEST(CacheTest, OneEntryGet) {
  Cache cache(10);

  char *a = reinterpret_cast<char *>(malloc(sizeof(char)));
  *a = 7;
  ASSERT_TRUE(cache.put("a", a, sizeof(char)));

  const char *a2 = NULL;
  size_t a2_size = 0;
  ASSERT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a2, &a2_size))));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(sizeof(char), a2_size);

  const char *a3 = NULL;
  size_t a3_size = 0;
  ASSERT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a3, &a3_size))));
  EXPECT_EQ(a, a3);
  EXPECT_EQ(*a, *a3); // test no seg fault
  EXPECT_EQ(sizeof(char), a3_size);
}

TEST(CacheTest, TwoEntryGet) {
  Cache cache(10);

  char *a = reinterpret_cast<char *>(malloc(sizeof(char)));
  *a = 7;
  ASSERT_TRUE(cache.put("a", a, sizeof(char)));

  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 18;
  ASSERT_TRUE(cache.put("b", b, sizeof(char)));

  const char *b2 = NULL;
  size_t b2_size = 0;
  ASSERT_TRUE(cache.get("b", newCB(bind(copyCacheData, _1, &b2, &b2_size))));
  EXPECT_EQ(b, b2);
  EXPECT_EQ(*b, *b2); // test no seg fault
  EXPECT_EQ(sizeof(char), b2_size);

  const char *a2 = NULL;
  size_t a2_size = 0;
  ASSERT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a2, &a2_size))));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(sizeof(char), a2_size);
}
  
TEST(CacheTest, DuplicatePut) {
  const size_t size = sizeof(int);
  Cache cache(3*size);

  int *a = reinterpret_cast<int *>(malloc(size));
  *a = 7;
  ASSERT_TRUE(cache.put("a", reinterpret_cast<char *>(a), size));

  int *a2 = reinterpret_cast<int *>(malloc(size));
  *a2 = 34;
  EXPECT_FALSE(cache.put("a", reinterpret_cast<char *>(a2), size));
  
  const char *a3 = NULL;
  size_t a3_size = 0;
  ASSERT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a3, &a3_size))));
  EXPECT_EQ(a, reinterpret_cast<const int *>(a3));
  EXPECT_EQ(*a, static_cast<char>(*a3)); // test no seg fault
  EXPECT_EQ(size, a3_size);

  // since a2 was not successfully inserted into cache
  delete(a2);
}

TEST(CacheTest, VarSizeEntries) {
  Cache cache(10);

  char *a = reinterpret_cast<char *>(malloc(2*sizeof(char)));
  memset(a, 7, 2*sizeof(char));
  ASSERT_TRUE(cache.put("key-a", a, 2*sizeof(char)));

  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 18;
  ASSERT_TRUE(cache.put("key-b", b, sizeof(char)));

  char *c = reinterpret_cast<char *>(malloc(3*sizeof(char)));
  memset(a, 33, 3*sizeof(char));
  ASSERT_TRUE(cache.put("key-c", c, 3*sizeof(char)));

  const char *c2 = NULL;
  size_t c2_size = 0;
  EXPECT_TRUE(cache.get("key-c", newCB(bind(copyCacheData, _1, &c2, &c2_size))));
  EXPECT_EQ(c, c2);
  EXPECT_EQ(*c, *c2); // test no seg fault
  EXPECT_EQ(3*sizeof(char), c2_size);
  
  const char *b2 = NULL;
  size_t b2_size = 0;
  EXPECT_TRUE(cache.get("key-b", newCB(bind(copyCacheData, _1, &b2, &b2_size))));
  EXPECT_EQ(b, b2);
  EXPECT_EQ(*b, *b2); // test no seg fault
  EXPECT_EQ(sizeof(char), b2_size);

  const char *a2 = NULL;
  size_t a2_size = 0;
  EXPECT_TRUE(cache.get("key-a", newCB(bind(copyCacheData, _1, &a2, &a2_size))));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(2*sizeof(char), a2_size);
}

TEST(CacheTest, DoneWithThenGet) {
  Cache cache(2);
  const char *key = "susi para kay A!";

  char *a = reinterpret_cast<char *>(malloc(2*sizeof(char)));
  memset(a, 7, 2*sizeof(char));
  ASSERT_TRUE(cache.put(key, a, 2*sizeof(char)));

  cache.doneWith(key);

  const char *a2 = NULL;
  size_t a2_size = 0;
  EXPECT_TRUE(cache.get(key, newCB(bind(copyCacheData, _1, &a2, &a2_size))));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(2*sizeof(char), a2_size);
}

TEST(CacheTest, InvalidDoneWithThenGet) {
  Cache cache(2);
  const char *key = "susi para kay A!";

  char *a = reinterpret_cast<char *>(malloc(2*sizeof(char)));
  memset(a, 7, 2*sizeof(char));
  ASSERT_TRUE(cache.put(key, a, 2*sizeof(char)));

  EXPECT_DEATH(cache.doneWith("Fake Key"), "Assertion");

  const char *a2 = NULL;
  size_t a2_size = 0;
  EXPECT_TRUE(cache.get(key, newCB(bind(copyCacheData, _1, &a2, &a2_size))));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(2*sizeof(char), a2_size);
}

TEST(CacheTest, ExceedQuota) {
  Cache cache(3);

  char *a = reinterpret_cast<char *>(malloc(2*sizeof(char)));
  memset(a, 7, 2*sizeof(char));
  ASSERT_TRUE(cache.put("a", a, 2*sizeof(char)));

  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 18;
  ASSERT_TRUE(cache.put("b", b, sizeof(char)));

  char *c = reinterpret_cast<char *>(malloc(3*sizeof(char)));
  memset(a, 33, 3*sizeof(char));
  ASSERT_TRUE(cache.put("c", c, 3*sizeof(char)));
  
  cache.doneWith("a");
  EXPECT_FALSE(cache.get("a", NULL));

  cache.doneWith("b");
  EXPECT_FALSE(cache.get("b", NULL));
  
  const char *c2;
  size_t c2_size = 0;
  ASSERT_TRUE(cache.get("c", newCB(bind(copyCacheData, _1, &c2, &c2_size))));
  EXPECT_EQ(c, c2);
  EXPECT_EQ(*c, *c2); // test no seg fault
  EXPECT_EQ(3*sizeof(char), c2_size);
}

TEST(CacheTest, TestRefCount) {
  Cache cache(1);
  
  char *a = reinterpret_cast<char *>(malloc(sizeof(char)));
  *a = 7;
  ASSERT_TRUE(cache.put("a", a, sizeof(char)));

  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 18;
  ASSERT_TRUE(cache.put("b", b, sizeof(char)));

  const char *a2;
  size_t a2_size = 0;
  EXPECT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a2, &a2_size))));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault

  cache.doneWith("a");

  const char *a3;
  size_t a3_size = 0;
  EXPECT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a3, &a3_size))));
  EXPECT_EQ(a, a3);
  EXPECT_EQ(*a, *a3); // test no seg fault

  cache.doneWith("a");
  cache.doneWith("a");
  EXPECT_DEATH(cache.doneWith("a"), "Assertion");
  EXPECT_FALSE(cache.get("a", NULL));

  const char *b2;
  size_t b2_size = 0;
  EXPECT_TRUE(cache.get("b", newCB(bind(copyCacheData, _1, &b2, &b2_size))));
  EXPECT_EQ(b, b2);
  EXPECT_EQ(*b, *b2); // test no seg fault
}

TEST(CacheTest, Reserve) {
  Cache cache(1);

  EXPECT_TRUE(cache.reserve("a"));
  EXPECT_FALSE(cache.reserve("a"));

  const char *a2;
  size_t a2_size = 0;
  EXPECT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a2, &a2_size))));

  const char *a3;
  size_t a3_size = 0;
  EXPECT_TRUE(cache.get("a", newCB(bind(copyCacheData, _1, &a3, &a3_size))));

  char *a = reinterpret_cast<char *>(malloc(sizeof(char)));
  *a = 7;
  ASSERT_TRUE(cache.put("a", a, sizeof(char)));

  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(a, a3);
  EXPECT_EQ(*a, *a3); // test no seg fault

  // Check ref count
  cache.doneWith("a");
  cache.doneWith("a");
  cache.doneWith("a");
  EXPECT_DEATH(cache.doneWith("a"), "Assertion");
}

TEST(CacheTest, CancelReservation) {
  Cache cache(1);

  EXPECT_TRUE(cache.reserve("a"));

  bool isCached1 = true; // expecting to turn false later on...
  EXPECT_TRUE(cache.get("a", newCB(bind(checkIfDataCached, _1, &isCached1))));

  bool isCached2 = true;
  EXPECT_TRUE(cache.get("a", newCB(bind(checkIfDataCached, _1, &isCached2))));
  
  EXPECT_FALSE(cache.cancelReservation("maling-susi"));
  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 7;
  ASSERT_TRUE(cache.put("b", b, sizeof(char)));
  
  EXPECT_TRUE(isCached1);
  EXPECT_TRUE(isCached2);

  EXPECT_TRUE(cache.cancelReservation("a"));
  EXPECT_FALSE(isCached1);
  EXPECT_FALSE(isCached2);
}

TEST(CacheTest, InvalidCancelReservation) {
  Cache cache(1);

  EXPECT_TRUE(cache.reserve("b"));

  const char *b2;
  size_t b2_size = 0;
  EXPECT_TRUE(cache.get("b", newCB(bind(copyCacheData, _1, &b2, &b2_size))));

  EXPECT_FALSE(cache.cancelReservation("a"));
  
  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 7;
  ASSERT_TRUE(cache.put("b", b, sizeof(char)));

  EXPECT_EQ(b, b2);
  EXPECT_EQ(*b, *b2); // test no seg fault  
}
