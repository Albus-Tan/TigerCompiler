#include "derived_heap.h"
#include <cstring>
#include <stack>
#include <stdio.h>

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap
//  correctly according to your design.
/**
 * Allocate a contiguous space from heap.
 * If your heap has enough space, just allocate and return.
 * If fails, return nullptr.
 * @param size size of the space allocated, in bytes.
 * @return start address of allocated space, if fail, return nullptr.
 */
char *DerivedHeap::Allocate(uint64_t size) { return free_list_.Allocate(size); }

char *DerivedHeap::AllocateRecord(uint64_t size, unsigned char *descriptor,
                                  uint64_t descriptor_size) {
  char *start = Allocate(size);
  if (start == nullptr)
    return nullptr;
  RecordInfo record_info(start, size, descriptor, descriptor_size);
  record_infos.push_back(record_info);
  return start;
}

char *DerivedHeap::AllocateArray(uint64_t size) {
  char *start = Allocate(size);
  if (start == nullptr)
    return nullptr;
  ArrayInfo array_info(start, size);
  array_infos.push_back(array_info);
  HEAP_LOG("AllocateArray start %lu, size %lu", (uint64_t)start, size)
  return start;
}

/**
 * Acquire the total allocated space from heap.
 * Hint: If you implement a contigous heap, you could simply calculate the
 * distance between top and bottom, but if you implement in a link-list way, you
 * may need to sum the whole free list and calculate the total free space. We
 * will evaluate your work through this, make sure you implement it correctly.
 * @return total size of the allocated space, in bytes.
 */
uint64_t DerivedHeap::Used() const { return size - free_list_.FreeSpace(); }

/**
 * Acquire the longest contiguous space from heap.
 * It should at least be 50% of inital heap size when initalization.
 * However, this may be tricky when you try to implement a Generatioal-GC, it
 * depends on how you set the ratio between Young Gen and Old Gen, we suggest
 * you making sure Old Gen is larger than Young Gen. Hint: If you implement a
 * contigous heap, you could simply calculate the distance between top and end,
 * if you implement in a link-list way, you may need to find the biggest free
 * list and return it. We use this to evaluate your utilzation, make sure you
 * implement it correctly.
 * @return size of the longest , in bytes.
 */
uint64_t DerivedHeap::MaxFree() const { return free_list_.MaxFree(); }

/**
 * Initialize your heap.
 * @param size size of your heap in bytes.
 */
void DerivedHeap::Initialize(uint64_t size) {
  HEAP_LOG("Initialize start")
  this->heap = (char *)malloc(size);
  this->size = size;
  free_list_.Init(heap, size);

  pointer_map_reader_ = new PointerMapReader();
  HEAP_LOG("Initialize finished")
}

void DerivedHeap::FreeHeap() {
  free(heap);
  delete pointer_map_reader_;
}

void DerivedHeap::ClearAllMarks() {
  for (auto &array : array_infos) {
    array.ClearMark();
  }
  for (auto &record : record_infos) {
    record.ClearMark();
  }
}

void DerivedHeap::Mark() {
  ClearAllMarks();
  // Mark phase:
  //  for each root v
  //    DFS(v)
#ifdef GC_DBG
  PrintStack();
#endif
  std::vector<uint64_t> roots_address =
      pointer_map_reader_->GetRootsAddress(stack);
  for (auto &root_address : roots_address) {
    DFS(root_address);
  }
}

bool DerivedHeap::IsPointer(uint64_t x) {
  // check if address x is a pointer
  for (auto &array : array_infos) {
    if (array.BetweenStartAndEnd(x))
      return true;
  }
  for (auto &record : record_infos) {
    if (record.BetweenStartAndEnd(x))
      return true;
  }
  return false;
}

bool DerivedHeap::MarkIfIsPointer(uint64_t x, RecordInfo **record_info_ptr) {
  // check if address x is a pointer
  // if is pointer, mark according structure
  for (auto &array : array_infos) {
    if (array.MarkIfBetweenStartAndEnd(x)) {
      HEAP_LOG("mark array, address %lu", x)
      return true;
    }
  }
  for (auto &record : record_infos) {
    if (record.MarkIfBetweenStartAndEnd(x)) {
      HEAP_LOG("mark record, address %lu", x)
      *record_info_ptr = &record;
      return true;
    }
  }
  return false;
}

void DerivedHeap::DFS(uint64_t x) {
  // function DFS(x)
  // if x is a pointer and points to record y
  //  if record y is not marked
  //	mark y
  //  for each field fi of record y
  // 	DFS(y.fi)
  RecordInfo *record_info_ptr = nullptr;
  if (MarkIfIsPointer(x, &record_info_ptr)) {
    if(record_info_ptr){
      uint64_t start_address = (uint64_t)((*record_info_ptr).start);
      for (uint64_t i = 0; i < (*record_info_ptr).descriptor_size; i++) {
        if ((*record_info_ptr).descriptor[i] == '1') {
          uint64_t field_address = *((uint64_t *)(start_address + WORD_SIZE * i));
          DFS(field_address);
        }
      }
    }
  }
}

void DerivedHeap::Sweep() {
  // Sweep phase:
  // p ← first address in heap
  // while p < last address in heap
  //     if record p is marked
  //	unmark p
  //     else let f1 be the first field in p
  //	p.f1 ← freelist
  //	freelist ← p
  //     p ← p + (size of record p)
  auto array_info_iter = array_infos.begin();
  while (array_info_iter != array_infos.end()) {
    if ((*array_info_iter).GetMark()) {
      // marked
      HEAP_LOG("array is marked, start %lu, size %lu",
               (uint64_t)(*array_info_iter).start, (*array_info_iter).size)
      (*array_info_iter).ClearMark();
      ++array_info_iter;
    } else {
      // not marked
      // add freelist
      free_list_.PushBack((*array_info_iter).start, (*array_info_iter).size);
      // delete from array_infos
      array_info_iter = array_infos.erase(array_info_iter);
    }
  }

  auto record_info_iter = record_infos.begin();
  while (record_info_iter != record_infos.end()) {
    if ((*record_info_iter).GetMark()) {
      // marked
      HEAP_LOG("record is marked, start %lu, size %lu",
               (uint64_t)(*record_info_iter).start, (*record_info_iter).size)
      (*record_info_iter).ClearMark();
      ++record_info_iter;
    } else {
      // not marked
      // add freelist
      free_list_.PushBack((*record_info_iter).start, (*record_info_iter).size);
      // delete from record_infos
      record_info_iter = record_infos.erase(record_info_iter);
    }
  }
}

/**
 * Do Garbage collection!
 * Hint: Though we do not suggest you implementing a Generational-GC due to
 * limited time, if you are willing to try it, add a function named YoungGC,
 * this will be treated as FullGC by default.
 */
void DerivedHeap::GC() {
  HEAP_LOG("GC start, array_infos size %zu, record_infos size %zu",
           array_infos.size(), record_infos.size())
  // Mark and Sweep
  Mark();
  Sweep();
  HEAP_LOG("GC finish, array_infos size %zu, record_infos size %zu",
           array_infos.size(), record_infos.size())
}

} // namespace gc
