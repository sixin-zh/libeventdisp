#include "gtest/gtest.h"
#include "cache.h"

using nyu_libedisp_webserver::Cache;

TEST(CacheTest, Empty) {
  Cache cache(10);
  char *c;
  size_t size;
  
  ASSERT_FALSE(cache.get("c", &c, size));
}

TEST(CacheTest, OneEntryGet) {
  Cache cache(10);

  char *a = reinterpret_cast<char *>(malloc(sizeof(char)));
  *a = 7;
  ASSERT_TRUE(cache.put("a", a, sizeof(char)));

  char *a2 = NULL;
  size_t a2_size;
  ASSERT_TRUE(cache.get("a", &a2, a2_size));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(sizeof(char), a2_size);

  char *a3 = NULL;
  size_t a3_size;
  ASSERT_TRUE(cache.get("a", &a3, a3_size));
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

  char *b2 = NULL;
  size_t b2_size;
  ASSERT_TRUE(cache.get("b", &b2, b2_size));
  EXPECT_EQ(b, b2);
  EXPECT_EQ(*b, *b2); // test no seg fault
  EXPECT_EQ(sizeof(char), b2_size);

  char *a2 = NULL;
  size_t a2_size;
  ASSERT_TRUE(cache.get("a", &a2, a2_size));
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
  
  char *a3 = NULL;
  size_t a3_size;
  ASSERT_TRUE(cache.get("a", &a3, a3_size));
  EXPECT_EQ(a, reinterpret_cast<int *>(a3));
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

  char *c2;
  size_t c2_size;
  EXPECT_TRUE(cache.get("key-c", &c2, c2_size));
  EXPECT_EQ(c, c2);
  EXPECT_EQ(*c, *c2); // test no seg fault
  EXPECT_EQ(3*sizeof(char), c2_size);
  
  char *b2;
  size_t b2_size;
  EXPECT_TRUE(cache.get("key-b", &b2, b2_size));
  EXPECT_EQ(b, b2);
  EXPECT_EQ(*b, *b2); // test no seg fault
  EXPECT_EQ(sizeof(char), b2_size);

  char *a2;
  size_t a2_size;
  EXPECT_TRUE(cache.get("key-a", &a2, a2_size));
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

  char *a2;
  size_t a2_size;
  EXPECT_TRUE(cache.get(key, &a2, a2_size));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(2*sizeof(char), a2_size);

  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 18;
  EXPECT_FALSE(cache.put("b", b, sizeof(char)));
  
  free(b);
}

TEST(CacheTest, InvalidDoneWithThenGet) {
  Cache cache(2);
  const char *key = "susi para kay A!";

  char *a = reinterpret_cast<char *>(malloc(2*sizeof(char)));
  memset(a, 7, 2*sizeof(char));
  ASSERT_TRUE(cache.put(key, a, 2*sizeof(char)));

  EXPECT_DEATH(cache.doneWith("Fake Key"), "Assertion");

  char *b = reinterpret_cast<char *>(malloc(sizeof(char)));
  *b = 18;
  EXPECT_FALSE(cache.put("b", b, sizeof(char)));
  
  char *a2;
  size_t a2_size;
  EXPECT_TRUE(cache.get(key, &a2, a2_size));
  EXPECT_EQ(a, a2);
  EXPECT_EQ(*a, *a2); // test no seg fault
  EXPECT_EQ(2*sizeof(char), a2_size);

  free(b);
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
  EXPECT_FALSE(cache.put("c", c, 3*sizeof(char)));
  
  cache.doneWith("a");
  EXPECT_FALSE(cache.put("c", c, 3*sizeof(char)));

  cache.doneWith("b");
  EXPECT_TRUE(cache.put("c", c, 3*sizeof(char)));

  char *a2;
  size_t a2_size = 0;
  EXPECT_FALSE(cache.get("a", &a2, a2_size));
  EXPECT_FALSE(cache.get("b", &a2, a2_size));
  
  char *c2;
  size_t c2_size;
  ASSERT_TRUE(cache.get("c", &c2, c2_size));
  EXPECT_EQ(c, c2);
  EXPECT_EQ(*c, *c2); // test no seg fault
  EXPECT_EQ(3*sizeof(char), c2_size);
}

