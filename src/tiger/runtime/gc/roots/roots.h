#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <cstdio>
#include <iostream>
#include <map>
#include <vector>

#include "tiger/frame/frame.h"



namespace gc {

#define GC_LOG(fmt, args...)                                            \
  do {                                                                         \
  } while (0);


//#define GC_LOG(fmt, args...)                                            \
//  do {                                                                         \
//    printf("[GC_LOG][%s:%d:%s] " fmt "\n", __FILE__, __LINE__,          \
//           __FUNCTION__, ##args);                                              \
//    fflush(stdout);                                                            \
//  } while (0);

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

// inserting for all function calls
class PointerMap {
public:
  std::string label; // label of current pointer map, "L" + return address label
  std::string next;  // next pointer map label
  std::string key;   // return address label
  std::string frame_size;           // stack size
  std::string is_tigermain = "0";   // is tigermain or not
  std::vector<std::string> offsets; // pointer offsets in frame
  std::string endmap = "-1";        // end map

  PointerMap() {}
};

class PointerMapGenerator {
private:
  std::vector<gc::PointerMap> global_roots;
public:
  PointerMapGenerator(){}

  // Examples of register maps in .data (x64)

  // GLOBAL_ROOTS:      % List head (global)
  // LL10:              % map label ("L" + return address label)
  //.quad LL11          % next, next pointer map label
  //.quad L10           % key, call's return address label
  //.quad 0x40          % frame(stack) size
  //.quad 0/1           % 1 for tigermain, 0 for not
  //.quad offset1       % pointer offset in frame
  //.quad offset2       % pointer offset in frame
  //.quad ...           % pointer offset in frame
  //.quad -1            % end map
  // ..............
  // LL11:
  //.quad LL10
  //.quad L11
  //.quad 0x20
  //.quad 8
  //.quad 0
  //.global GLOBAL_ROOTS

  // output pointer map in .data section
  void Print(FILE *out, bool is_last) {
    GC_LOG("output pointer map in .data section")
    // last entry
    if (is_last)
      global_roots.back().next = "0";
    // Output pointerMap
    for (PointerMap map : global_roots) {
      std::string out_str;
      out_str.append(map.label + ":\n");
      out_str.append(".quad " + map.next + "\n");
      out_str.append(".quad " + map.key + "\n");
      out_str.append(".quad " + map.frame_size + "\n");
      out_str.append(".quad " + map.is_tigermain + "\n");
      for (std::string &offset : map.offsets)
        out_str.append(".quad " + offset + "\n");
      out_str.append(".quad " + map.endmap + "\n");
      fprintf(out, "%s", out_str.c_str());
    }
  }

  void PushBackPointerMap(const PointerMap& pointer_map){
    global_roots.push_back(pointer_map);
  }

  // add next pointer
  void LinkPointerMaps(){
    for (int i = 0; i < (int)global_roots.size() - 1; i++)
      global_roots[i].next = global_roots[i + 1].label;
  }

};

class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
  PointerMapGenerator *pointer_map_generator_;
  frame::Frame *frame_;
  assem::InstrList *instr_list_;
  std::vector<int> pointer_offsets;
  std::map<assem::Instr *, std::vector<int>> return_address_map_offsets;
public:
  Roots(frame::Frame *frame, assem::InstrList *instr_list)
      : frame_(frame), instr_list_(instr_list) {
    pointer_offsets = frame_->GetPointerOffsets();
    pointer_map_generator_ = new PointerMapGenerator();
  }

  PointerMapGenerator *GetPointerMapGenerator(){
    return pointer_map_generator_;
  }


  // generate pointer map and add in global_roots
  void GenPointerMaps() {
    GC_LOG("step in, frame %s", frame_->name_->Name().data())
    bool generate = false;
    for(auto instr: instr_list_->GetList()){
      if (!generate && typeid(*instr) == typeid(assem::OperInstr)) {
        std::string ass = static_cast<assem::OperInstr *>(instr)->assem_;
        if (ass.find("call") == ass.npos) {
          generate = false;
          continue;
        }
        generate = true;
        continue;
      }
      if(generate && typeid(*instr) == typeid(assem::LabelInstr)){
        PointerMap pointer_map;
        pointer_map.frame_size = frame_->name_->Name() + "_framesize";
        // FIXME
        pointer_map.key =
            static_cast<assem::LabelInstr *>(instr)->label_->Name();
        GC_LOG("return label %s", pointer_map.key.data())
        pointer_map.label = "L" + pointer_map.key;
        pointer_map.next = "0";
        if (frame_->name_->Name() == "tigermain")
          pointer_map.is_tigermain = "1";

        for (int offset : pointer_offsets)
          pointer_map.offsets.push_back(std::to_string(offset));

        pointer_map_generator_->PushBackPointerMap(pointer_map);
      }
    }
    pointer_map_generator_->LinkPointerMaps();
  };
};

} // namespace gc

#endif // TIGER_RUNTIME_GC_ROOTS_H