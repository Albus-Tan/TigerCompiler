#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {

//#define DBG_COL

//#define COL_LOG(fmt, args...)                                            \
//  do {                                                                         \
//    printf("[COL_LOG][%s:%d:%s] " fmt "\n", __FILE__, __LINE__,          \
//           __FUNCTION__, ##args);                                              \
//    fflush(stdout);                                                            \
//  } while (0);

#define COL_LOG(fmt, args...)                                            \
  do {                                                                         \
  } while (0);

/* TODO: Put your lab6 code here */
col::Result Color::BuildAndGetResult() {
  return {coloring, new live::INodeList()};
}

Color::Color() {
  coloring = temp::Map::Empty();
  auto regs = reg_manager->Registers()->GetList();
  for (auto reg : regs) {
    auto reg_name = reg_manager->temp_map_->Look(reg);
    okColors.insert(reg_name);
    // precolored nodes
    coloring->Enter(reg, reg_name);
    COL_LOG("insert precolored temp %d, color %s", reg->Int(), reg_name->data())
#ifdef DBG_COL
    COL_LOG("Look result %s", coloring->Look(reg)->data())
#endif
  }
}

void Color::RemoveOkColor(live::INodePtr n) {
  // okColors ← okColors \ {color[n]}
  okColors.erase(coloring->Look(n->NodeInfo()));
}

bool Color::AssignColor(live::INodePtr n) {
  if(okColors.empty()) return false;
  // let c ∈ okColor
  auto c = okColors.begin();
  // color[n] ← c
  coloring->Enter(n->NodeInfo(), *c);
  COL_LOG("AssignColor %s to temp %d",(*c)->data(),n->NodeInfo()->Int())
  return true;
}

void Color::AssignSameColor(live::INodePtr color_src,
                            live::INodePtr color_dst) {
  COL_LOG("start finding temp %d color", color_src->NodeInfo()->Int())
  auto c = coloring->Look(color_src->NodeInfo());
  // incase not finding color of src
  assert(c);
  coloring->Enter(color_dst->NodeInfo(), c);
}

void Color::InitOkColors() {
  okColors.clear();
  for (temp::Temp *temp : reg_manager->Registers()->GetList()) {
    std::string *color = reg_manager->temp_map_->Look(temp);
    okColors.insert(color);
  }
}
bool Color::OkColorsEmpty() { return okColors.empty(); }

} // namespace col
