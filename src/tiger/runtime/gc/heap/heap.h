#pragma once

#include <stdint.h>
#include <iostream>

// Used to locate the start of ptrmap, simply get the address by &GLOBAL_GC_ROOTS
extern uint64_t GLOBAL_GC_ROOTS;

#define GET_RBP(rbp) do { __asm__("movq %%rbp, %0" : "=r"(rbp)); } while(0)

// Used to get the first tiger stack, define a sp like uint64_t *sp; and invoke GET_TIGER_STACK(sp) will work
#define GET_TIGER_STACK(sp) do {\
  unsigned long rbp; \
  GET_RBP(rbp); \ 
  sp = ((uint64_t*)((*(uint64_t*)rbp) + sizeof(uint64_t))); \
} while(0)

namespace gc {

constexpr long END_MARK = 0;

class TigerHeap {
public:
  /**
   * Allocate a contiguous space from heap.
   * If your heap has enough space, just allocate and return.
   * If fails, return nullptr.
   * @param size size of the space allocated, in bytes.
   * @return start address of allocated space, if fail, return nullptr.
   */
  virtual char *Allocate(uint64_t size) = 0;

  /**
   * Acquire the total allocated space from heap.
   * Hint: If you implement a contigous heap, you could simply calculate the distance between top and bottom,
   * but if you implement in a link-list way, you may need to sum the whole free list and calculate the total free space.
   * We will evaluate your work through this, make sure you implement it correctly.
   * @return total size of the allocated space, in bytes.
   */
  virtual uint64_t Used() const = 0;

  /**
   * Acquire the longest contiguous space from heap.
   * It should at least be 50% of inital heap size when initalization.
   * However, this may be tricky when you try to implement a Generatioal-GC, it depends on how you set the ratio
   * between Young Gen and Old Gen, we suggest you making sure Old Gen is larger than Young Gen.
   * Hint: If you implement a contigous heap, you could simply calculate the distance between top and end,
   * if you implement in a link-list way, you may need to find the biggest free list and return it.
   * We use this to evaluate your utilzation, make sure you implement it correctly.
   * @return size of the longest , in bytes.
   */
  virtual uint64_t MaxFree() const = 0;

  /**
   * Initialize your heap.
   * @param size size of your heap in bytes.
   */
  virtual void Initialize(uint64_t size) = 0;

  /**
   * Do Garbage collection!
   * Hint: Though we do not suggest you implementing a Generational-GC due to limited time,
   * if you are willing to try it, add a function named YoungGC, this will be treated as FullGC by default.
   */
  virtual void GC() = 0;

  static constexpr uint64_t WORD_SIZE = 8;
};

} // namespace gc

