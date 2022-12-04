#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */
std::unique_ptr<col::Result> Color::BuildAndTransferResult() {
  result_ = std::make_unique<col::Result>(coloring, new live::INodeList());
  return std::move(result_);
}

Color::Color() {
  auto regs = reg_manager->Registers()->GetList();
  for(auto reg: regs){
    auto reg_name = reg_manager->temp_map_->Look(reg);
    okColors.insert(reg_name);
    coloring->Enter(reg, reg_name);
  }
}
} // namespace col
