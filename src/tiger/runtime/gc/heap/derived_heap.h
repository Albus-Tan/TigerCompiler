#pragma once

#include "heap.h"
#include <list>
#include <vector>

namespace gc {

class FreeListNode {
public:
  char *start = nullptr;
  uint64_t size = 0;
  FreeListNode() {}
  FreeListNode(char *start, uint64_t size) : start(start), size(size) {}
};

class FreeList {
  std::list<FreeListNode> free_list;

public:
  void Init(char *heap, uint64_t size) { free_list.push_back({heap, size}); }

  /**
   * Allocate a contiguous space from heap.
   * If your heap has enough space, just allocate and return.
   * If fails, return nullptr.
   * @param size size of the space allocated, in bytes.
   * @return start address of allocated space, if fail, return nullptr.
   */
  char *Allocate(uint64_t size) {
    if (free_list.empty())
      return nullptr;
    if (size > MaxFree()) {
      HEAP_LOG("size > MaxFree(), Allocate return nullptr");
      return nullptr;
    }
    char *ret_add = nullptr;
    auto it = free_list.begin();
    for (; it != free_list.end(); ++it) {
      if ((*it).size >= size) {
        // HEAP_LOG("Need size %lu, select %lu", size, (*it).size)
        ret_add = (*it).start;
        auto rest_size = (*it).size - size;
        char *new_start = (*it).start + size;

        // remove current node from list
        free_list.erase(it);

        // add rest part to freelist
        if (rest_size > 0) {
          free_list.push_back({new_start, rest_size});
        }
        return ret_add;
      }
    }
  }

  uint64_t FreeSpace() const {
    uint64_t free_space = 0;
    for (auto &node : free_list) {
      free_space += node.size;
    }
    HEAP_LOG("FreeSpace, free space %lu", free_space)
    return free_space;
  }

  uint64_t MaxFree() const {
    uint64_t max_free = 0;
    for (auto &node : free_list) {
      max_free = std::max(node.size, max_free);
    }
    return max_free;
  }

public:
  FreeList() {}

  void PushBack(char *start, uint64_t size) {
    free_list.push_back({start, size});
  }
  void PushFront(char *start, uint64_t size) {
    free_list.push_front({start, size});
  }
  bool Empty() { return free_list.empty(); }
  void Clear() { free_list.clear(); }
  const std::list<FreeListNode> &GetList() { return free_list; }
};

class RecordInfo {
private:
  bool marked = false;

public:
  char *start = nullptr;
  uint64_t size = 0;
  unsigned char *descriptor = nullptr; // typ_->Name() + "_DESCRIPTOR"
  uint64_t descriptor_size = 0;
  bool BetweenStartAndEnd(uint64_t address) {
    if (size <= 0)
      return false;
    // check whether the address is between start and start + size
    // then the pointer address points to record or a field of it
    int64_t delta = (int64_t)address - (int64_t)start;
    return (delta >= 0 && delta <= (size - 8));
  }
  bool MarkIfBetweenStartAndEnd(uint64_t address) {
    if (BetweenStartAndEnd(address)) {
      // no need to mark again if already marked
      if (marked)
        return false;
      Mark();
      return true;
    }
    return false;
  }
  RecordInfo() {}
  RecordInfo(char *start, uint64_t size) : start(start), size(size) {}
  RecordInfo(char *start, uint64_t size, unsigned char *descriptor,
             uint64_t descriptor_size)
      : start(start), size(size), descriptor(descriptor),
        descriptor_size(descriptor_size) {}
  void Mark() { marked = true; }
  void ClearMark() { marked = false; }
  bool GetMark() const { return marked; }
};

class ArrayInfo {
private:
  bool marked = false;

public:
  char *start = nullptr;
  uint64_t size = 0;
  bool BetweenStartAndEnd(uint64_t address) {
    if (size <= 0)
      return false;
    // check whether the address is between start and start + size
    // then the pointer address points to record or a field of it
    int64_t delta = (int64_t)address - (int64_t)start;
    return (delta >= 0 && delta <= (size - 8));
  }
  bool MarkIfBetweenStartAndEnd(uint64_t address) {
    if (BetweenStartAndEnd(address)) {
      // no need to mark again if already marked
      if (marked)
        return false;
      Mark();
      return true;
    }
    return false;
  }
  ArrayInfo() {}
  ArrayInfo(char *start, uint64_t size) : start(start), size(size) {}
  void Mark() { marked = true; }
  void ClearMark() { marked = false; }
  bool GetMark() const { return marked; }
};

// pointer map after linked
class LinkedPointerMap {
public:
  uint64_t label =
      0; // address of current pointer map ("L" + return address label)
  uint64_t next = 0;            // next pointer map address
  uint64_t key = 0;             // call's return address
  uint64_t frame_size = 0;      // stack size
  uint64_t is_tigermain = 0;    // is tigermain or not
  std::vector<int64_t> offsets; // pointer offsets in frame
  LinkedPointerMap() {}
  void Print() {
    std::cout << "[label] " << label << std::endl;
    std::cout << "[next] " << next << std::endl;
    std::cout << "[return address] " << key << std::endl;
    std::cout << "[frame size] " << frame_size << std::endl;
    std::cout << "[is tigermain] " << (bool)(is_tigermain) << std::endl;
    for (auto &offset : offsets) {
      std::cout << "[offset] " << offset << std::endl;
    }
  }
};

class PointerMapReader {
private:
  std::vector<LinkedPointerMap> linked_pointer_maps;
  const uint64_t WORD_SIZE = 8;

public:
  PointerMapReader() { ReadPointerMaps(); }
  void ReadPointerMaps() {
    HEAP_LOG("ReadPointerMaps start")
    // Used to locate the start of ptrmap, simply get the address by
    uint64_t *pointer_map_start = &GLOBAL_GC_ROOTS;
    HEAP_LOG("pointer_map_start %lu", *pointer_map_start)
    uint64_t *tmp = pointer_map_start;
    while (true) {
      LinkedPointerMap linked_pointer_map;
      HEAP_LOG("------------------")
      linked_pointer_map.label = (uint64_t)tmp;
      HEAP_LOG("label %lu", linked_pointer_map.label)
      //.quad LL11          % next, next pointer map label
      linked_pointer_map.next = *tmp;
      HEAP_LOG("next %lu", linked_pointer_map.next)
      ++tmp;
      //.quad L10           % key, call's return address label
      linked_pointer_map.key = *tmp;
      HEAP_LOG("key %lu", linked_pointer_map.key)
      ++tmp;
      //.quad 0x40          % frame(stack) size
      linked_pointer_map.frame_size = *tmp;
      HEAP_LOG("frame_size %lu", linked_pointer_map.frame_size)
      ++tmp;
      //.quad 0/1           % 1 for tigermain, 0 for not
      linked_pointer_map.is_tigermain = *tmp;
      HEAP_LOG("is_tigermain %lu", linked_pointer_map.is_tigermain)
      ++tmp;
      //.quad offset1       % pointer offset in frame
      while (true) {
        int64_t offset = *tmp;
        HEAP_LOG("offset %lu", offset)
        ++tmp;
        if (offset == -1) //.quad -1            % end map
          break;
        else {
          linked_pointer_map.offsets.push_back(offset);
        }
      }
      HEAP_LOG("finish current map")
      linked_pointer_maps.push_back(linked_pointer_map);
      if (linked_pointer_map.next == 0) {
        HEAP_LOG("END")
        break;
      }
    }
#ifdef GC_DBG
    Print();
#endif
  }

  std::vector<uint64_t> GetRootsAddress(uint64_t *sp) {
    HEAP_LOG("GetRootsAddress start")
    HEAP_LOG("sp %lu", (uint64_t)sp)
    std::vector<uint64_t> roots_address;
    bool is_tigermain = false;
    while (!is_tigermain) {
      uint64_t return_address = *(sp - 1);
      HEAP_LOG("return_address %lu", return_address)
      for (auto &pm : linked_pointer_maps) {
        if (pm.key == return_address) {
          HEAP_LOG("return_address %lu found in pointer map", return_address)
          for (int64_t offset : pm.offsets) {
            // sp + framesize + offset
            // offset is negative, so use (int64_t)
            // instead of (uint_64_t) when adding
            uint64_t *pointer_address =
                (uint64_t *)(offset + (int64_t)sp + (int64_t)pm.frame_size);
            roots_address.push_back(*pointer_address);
            HEAP_LOG("pointer_address %lu at %lu of offset %ld",
                     *pointer_address, (uint64_t)pointer_address, offset)
          }
          sp += (pm.frame_size / WORD_SIZE + 1);
          is_tigermain = (bool)pm.is_tigermain;
          break;
        }
      }
    }
    return roots_address;
  }

  void Print() {
    std::cout << "Print Linked Pointer Map Read" << std::endl;
    std::cout << "------------START------------" << std::endl;
    for (auto &lpm : linked_pointer_maps) {
      std::cout << "-------------------------" << std::endl;
      lpm.Print();
    }
    std::cout << "------------END------------" << std::endl;
  }
};

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap
  // correctly according to your design.
private:
  FreeList free_list_;
  std::list<RecordInfo> record_infos;
  std::list<ArrayInfo> array_infos;
  PointerMapReader *pointer_map_reader_;

  void ClearAllMarks();
  void Mark();
  void Sweep();
  void DFS(uint64_t x);
  bool IsPointer(uint64_t x);
  bool MarkIfIsPointer(uint64_t x, RecordInfo **record_info_ptr);

  // Debug
  void PrintStack(){
    uint64_t *sp = stack;
    for(int i = 0; i < 80; ++i){
      HEAP_LOG("sp %lu, info %lu", (uint64_t)sp, *sp)
      sp += 1;
    }
  }

public:
  char *AllocateRecord(uint64_t size, unsigned char *descriptor,
                       uint64_t descriptor_size) override;
  char *AllocateArray(uint64_t size) override;
  char *Allocate(uint64_t size) override;

  uint64_t Used() const override;

  uint64_t MaxFree() const override;

  void Initialize(uint64_t size) override;

  void GC() override;

  void FreeHeap() override;
};

} // namespace gc
