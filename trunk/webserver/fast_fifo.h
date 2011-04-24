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

#ifndef LIBEVENTDISP_FAST_FIFO_H_
#define LIBEVENTDISP_FAST_FIFO_H_

#include <cassert>

namespace nyu_libedisp_webserver {
template <typename T> class FastFIFO;

// A node for FastFIFO
template <typename T>
class FastFIFONode {
 public:
  // Creates a node initialized with a given data.
  FastFIFONode(T *data);

 private:
  friend class FastFIFO<T>;
  T *data;
  FastFIFONode *before;
  FastFIFONode *next;
};

// A simple templetized FIFO implementation.
// Invariant: tail is always pointing to a sentinel
template <typename T>
class FastFIFO {
 public:
  FastFIFO(void);
  ~FastFIFO();

  // Remove the node from this FIFO.
  //
  // Param:
  //  node - the node to remove
  //
  // WARNING: This method is very dangerous! Always be careful and make sure
  //   that the node really belongs to this object.
  void removeNode(FastFIFONode<T> *node);

  // Push a data to this object.
  //
  // Param:
  //  data - the data to store. The client is still responsible for the
  //    lifecycle of the data.
  //
  // Returns the node that contains the data
  FastFIFONode<T>* push(T *data);

  // Pops a node at the front of this object.
  //
  // Param:
  //  data - the data contained on the node will be assigned here when pop was
  //    successful.
  //
  // Returns true if pop was successful.
  bool pop(T **data);
  
 private:
  FastFIFONode<T> *tail_;
  FastFIFONode<T> *head_;

  // Disallow copying
  FastFIFO(const FastFIFO<T>&);
  void operator=(const FastFIFO<T>&);
};

template <typename T>
FastFIFONode<T>::FastFIFONode(T *data) : data(data), before(NULL), next(NULL) {
}

template <typename T>
FastFIFO<T>::FastFIFO(void) : tail_(new FastFIFONode<T>(NULL)), head_(tail_) {
}

template <typename T>
FastFIFO<T>::~FastFIFO(void) {
  T *dummy;
  while (pop(&dummy));
}

template <typename T>
FastFIFONode<T>* FastFIFO<T>::push(T *data) {
  FastFIFONode<T> *newNode = new FastFIFONode<T>(data);

  newNode->before = tail_->before;
  newNode->next = tail_;

  if (tail_->before != NULL) {
    tail_->before->next = newNode;
  }

  tail_->before = newNode;

  if (head_ == tail_) {
    head_ = newNode;
  }

  return newNode;
}

template <typename T>
bool FastFIFO<T>::pop(T **data) {
  bool ret = false;
  
  if (head_ != tail_) {
    *data = head_->data;
    removeNode(head_);    
    ret = true;
  }

  return ret;
}

template <typename T>
void FastFIFO<T>::removeNode(FastFIFONode<T> *node) {
  assert(node != NULL);
  
  FastFIFONode<T> *before = node->before;
  FastFIFONode<T> *next = node->next;

  // Check if node is not malformed. next will never be NULL because of the
  // sentinel node at the tail.
  assert(next != NULL);
  
  if (before == NULL) {
    head_ = next;
  }
  else {
    before->next = next;
  }

  next->before = before;
  delete node;
}

}

#endif

