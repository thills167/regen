/*
 * memory-allocator.h
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#ifndef MEMORY_ALLOCATOR_H_
#define MEMORY_ALLOCATOR_H_

#include <stdlib.h>

namespace regen {
  // TODO: howto use new/delete/malloc/glBufferData/...
  // TODO: possible to use this for cpp classes ?

  /**
   * \brief A pool of memory allocators.
   *
   * Each allocator manages contiguous pre-allocated memory.
   * If no allocator can handle a request a new allocator is
   * automatically created.
   * Allocators must implement alloc,free,size,maxSpace and a constructor
   * taking the number of managed bytes.
   */
  template<typename AllocatorType,
           typename ReferenceType> class AllocatorPool
  {
  public:
    /**
     * \brief Doubly linked node containing an allocator.
     */
    struct Node {
      /**
       * @param size the allocator size.
       */
      Node(unsigned int size) : allocator(size) {}
      AllocatorType allocator; //!< the allocator
      Node *prev;              //!< allocator with bigger maxSpace
      Node *next;              //!< allocator with smaller maxSpace
    };
    /**
     * \brief Reference to allocated memory.
     */
    struct Reference {
      /**
       * @param allocator the allocator.
       */
      Reference(const AllocatorType *allocator);
      Node *allocatorNode;         //!< the allocator
      ReferenceType allocatorRef;  //!< the allocator reference for fast removal
    };

    AllocatorPool()
    : allocators_(NULL), defaultSize_(4*1024*1024)
    {}
    ~AllocatorPool()
    {
      for(Node *n=allocators_; n!=NULL;)
      {
        Node *buf = n;
        n = n->next;
        delete buf;
      }
    }

    /**
     * @param size min size of automatically instantiated allocators.
     */
    void set_defaultSize(unsigned int size)
    { defaultSize_ = size; }

    /**
     * Instantiate a new allocator and add it to the pool.
     * @param size the allocator size.
     */
    void createAllocator(unsigned int size)
    {
      allocators_->prev = new Node(size);
      allocators_->prev->prev = NULL;
      allocators_->prev->next = allocators_;
      allocators_ = allocators_->prev;
      sortInForward(allocators_);
    }

    /**
     * Allocate memory managed by an allocator.
     * @param size number of bytes to allocate.
     * @return reference for fast removal
     */
    Reference alloc(unsigned int size)
    {
      AllocatorPool::Reference ref;

      // find allocator with smallest maxSpace and maxSpace>size
      Node *min=NULL;
      for(Node *n=allocators_; n->allocator.maxSpace()>size; n=n->next)
      { min = n; }

      if(min==NULL) {
        createAllocator( max(size,defaultSize_) );
        ref.allocatorRef = allocators_->allocator.alloc(size);
        ref.allocatorNode = allocators_;
        sortInForward(allocators_);
      }
      else {
        ref.allocatorRef = min->allocator.alloc(size);
        ref.allocatorNode = min;
        sortInForward(min);
      }

      return ref;
    }

    /**
     * Free previously allocated memory.
     * @param ref reference for fast removal
     */
    void free(const Reference &ref)
    {
      ref.allocatorNode->allocator->free(ref.allocatorRef);
      sortInBackward(ref.allocatorNode);
    }

  protected:
    Node *allocators_;
    unsigned int defaultSize_;

    void sortInForward(Node *resizedNode)
    {
      unsigned int space = resizedNode->allocator.maxSpace();
      for(Node *n=resizedNode->next;
          n!=NULL && n->allocator.maxSpace()>space;
          n=n->next)
      { swap(n->prev,n); }
    }
    void sortInBackward(Node *resizedNode)
    {
      unsigned int space = resizedNode->allocator.maxSpace();
      for(Node *n=resizedNode->prev;
          n!=NULL && n->allocator.maxSpace()<space;
          n=n->prev)
      { swap(n->next,n); }
    }

    void swap(Node *n0, Node *n1)
    {
      if(n1->next) n1->next->prev=n0;
      if(n0->prev) n0->prev->next=n1;
      n1->prev = n0->prev;
      n0->next = n1->next;
      n0->prev = n1;
      n1->next = n0;
    }
  };

  /////////////////////////////////
  /////////////////////////////////
  /////////////////////////////////

  /**
   * \brief Implements a variant of the buddy memory allocation algorithm.
   *
   * Dynamic memory allocation is not cheap. GPU memory is precious.
   * For these reasons this class provides memory management with
   * the intention to provide fast alloc() and free() functions
   * and keeping the fragmentation in the memory as low as possible without
   * moving any memory.
   *
   * The algorithm uses a binary tree to partition the pre-allocated memory.
   * When memory is allocated the algorithm searches for a �free� node that
   * offers enough space for the request.
   * The chosen node is cut in halves until half the node size does not
   * fit the request anymore. Then the node is cut into one �full� node that
   * fits the request exactly and another  �free� node for the remaining space.
   * No internal fragmentation occurs using this implementation.
   * External fragmentation can happen when partitions are to small
   * to fit allocation requests for a long time.
   * Allocating some relative small chunks of memory helps in keeping the
   * fragmentation costs low.
   *
   * Addresses are only used relative to the start address within the allocator.
   * If the allocated memory refers to any RAM you have to offset the
   * pre-allocated data pointer with the relative addresses used
   * within this class.
   */
  class BuddyAllocator
  {
  public:
    /**
     * The current allocator state.
     */
    enum State {
      FREE,  //!< no allocate active
      FULL,  //!< no more allocate possible
      PARTIAL//!< some allocates active but space left
    };

    /**
     * @param size number of pre-allocated bytes.
     */
    BuddyAllocator(unsigned int size);

    /**
     * @return The current allocator state.
     */
    State allocaterState() const;
    /**
     * @return number of pre-allocated bytes.
     */
    unsigned int size() const;
    /**
     * @return maximum contiguous space that can be allocated.
     */
    unsigned int maxSpace() const;

    /**
     * Allocate memory managed by the allocator.
     * @param size number of bytes to allocate.
     * @param addressRet relative address of allocated memory
     * @return false if not enough space left.
     */
    bool alloc(unsigned int size, unsigned int *addressRet);
    /**
     * Free previously allocated memory.
     * @param address address of the memory block to free
     */
    void free(unsigned int address);

  protected:
    struct BuddyNode {
      BuddyNode(
          unsigned int address,
          unsigned int size,
          BuddyNode *parent);
      State state;
      unsigned int address;
      unsigned int size;
      unsigned int maxSpace;
      BuddyNode *leftChild;
      BuddyNode *rightChild;
      BuddyNode *parent;
    };
    BuddyNode *buddyTree_;

    unsigned int createPartition(BuddyNode *n, unsigned int size);
    void computeMaxSpace(BuddyNode *n);
  };
  typedef AllocatorPool<BuddyAllocator, unsigned int> BuddyAllocatorPool;
}

#endif /* MEMORY_ALLOCATOR_H_ */
