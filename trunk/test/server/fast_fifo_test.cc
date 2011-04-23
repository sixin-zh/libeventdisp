#include "gtest/gtest.h"
#include "fast_fifo.h"

using nyu_libedisp_webserver::FastFIFONode;
using nyu_libedisp_webserver::FastFIFO;

TEST(FastFIFOTest, Empty) {
  FastFIFO<int> fifo;
  int *x;
  EXPECT_FALSE(fifo.pop(&x));
}

TEST(FastFIFOTest, SimplePushPop) {
  FastFIFO<int> fifo;

  int x = 1;
  fifo.push(&x);

  int y = 2;
  fifo.push(&y);

  int *x2, *y2;
  
  EXPECT_TRUE(fifo.pop(&x2));
  EXPECT_EQ(x, *x2);
  EXPECT_TRUE(fifo.pop(&y2));
  EXPECT_EQ(y, *y2);
  EXPECT_FALSE(fifo.pop(&x2));
}

TEST(FastFIFOTest, EmptyAndRefill) {
  FastFIFO<int> fifo;

  int x = 1;
  fifo.push(&x);
  fifo.push(&x);

  int *x2;
  fifo.pop(&x2);
  fifo.pop(&x2);
  EXPECT_FALSE(fifo.pop(&x2));

  // fifo is now empty, try to put something again
  int y = 2;
  fifo.push(&y);

  int z = 3;
  fifo.push(&z);

  int *y2, *z2;
  EXPECT_TRUE(fifo.pop(&y2));
  EXPECT_EQ(y, *y2);
  EXPECT_TRUE(fifo.pop(&z2));
  EXPECT_EQ(z, *z2);
  EXPECT_FALSE(fifo.pop(&x2));
}

TEST(FastFIFOTest, RemoveAtFront) {
  FastFIFO<int> fifo;

  int x = 1;
  FastFIFONode<int> *node = fifo.push(&x);

  int y = 2;
  fifo.push(&y);

  fifo.removeNode(node);

  int *y2;
  EXPECT_TRUE(fifo.pop(&y2));
  EXPECT_EQ(y, *y2);
  EXPECT_FALSE(fifo.pop(&y2));
}

TEST(FastFIFOTest, RemoveAtBack) {
  FastFIFO<int> fifo;

  int x = 1;
  fifo.push(&x);

  int y = 2;
  FastFIFONode<int> *node = fifo.push(&y);

  fifo.removeNode(node);
  
  int *x2;
  EXPECT_TRUE(fifo.pop(&x2));
  EXPECT_EQ(x, *x2);
  EXPECT_FALSE(fifo.pop(&x2));
}

TEST(FastFIFOTest, RemoveAtMid) {
  FastFIFO<int> fifo;

  int x = 1;
  fifo.push(&x);

  int y = 2;
  FastFIFONode<int> *node = fifo.push(&y);

  int z = 3;
  fifo.push(&z);

  fifo.removeNode(node);

  int *x2, *z2;
  
  EXPECT_TRUE(fifo.pop(&x2));
  EXPECT_EQ(x, *x2);
  EXPECT_TRUE(fifo.pop(&z2));
  EXPECT_EQ(z, *z2);
  EXPECT_FALSE(fifo.pop(&x2));
}

