#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  auto *list = new assem::InstrList();
  for (auto stm : traces_->GetStmList()->GetList()){
    /* TODO: Put your lab5 code here */
    // stm->Munch(*list);
  }

  assem_instr_ =
      std::make_unique<AssemInstr>(frame::ProcEntryExit2(list));

}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(
      new assem::LabelInstr(temp::LabelFactory::LabelString(label_), label_));

}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

} // namespace tree
