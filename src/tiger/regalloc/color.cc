#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
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
  return true;
}

void Color::AssignSameColor(live::INodePtr color_src,
                            live::INodePtr color_dst) {
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
