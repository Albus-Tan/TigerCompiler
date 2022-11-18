#include "tiger/frame/x64frame.h"
#include "frame.h"

extern frame::RegManager *reg_manager;

namespace frame {

int X64Frame::AllocLocal() {
  // Keep away from the return address on the top of the frame
  offset_ -= reg_manager->WordSize();
  return offset_;
}

X64Frame::X64Frame(temp::Label *name, std::list<bool> formals): Frame(name){
  formals_ = new std::list<frame::Access *>();
  for(auto formal_escape : formals){
    formals_->push_back(frame::Access::AllocLocal(this, formal_escape));
  }
}

std::list<frame::Access *> *X64Frame::Formals() { return nullptr; }

/* TODO: Put your lab5 code here */
temp::TempList *X64RegManager::Registers() {
  /* TODO: Put your lab5 code here */
  temp::TempList *temp_list = {};
  for (std::string reg_name : X64RegNames) {
    if (reg_name == "rsi") {
      continue;
    }
  }
  return nullptr;
}

temp::TempList *X64RegManager::ArgRegs() {
  /* TODO: Put your lab5 code here */
  return nullptr;
}

temp::TempList *X64RegManager::CallerSaves() {
  /* TODO: Put your lab5 code here */
  return nullptr;
}

temp::TempList *X64RegManager::CalleeSaves() {
  /* TODO: Put your lab5 code here */
  return nullptr;
}

temp::TempList *X64RegManager::ReturnSink() {
  /* TODO: Put your lab5 code here */
  temp::TempList *temp_list = CalleeSaves();
  temp_list->Append(StackPointer());
  temp_list->Append(ReturnValue());
  return temp_list;
}

int X64RegManager::WordSize() {
  /* TODO: Put your lab5 code here */
  return 0;
}

temp::Temp *X64RegManager::FramePointer() {
  /* TODO: Put your lab5 code here */
  return nullptr;
}

temp::Temp *X64RegManager::StackPointer() {
  /* TODO: Put your lab5 code here */
  return nullptr;
}

temp::Temp *X64RegManager::ReturnValue() {
  /* TODO: Put your lab5 code here */
  return nullptr;
}
X64RegManager::X64RegManager() : RegManager() {

  for (std::string reg_name : X64RegNames) {
    temp::Temp *reg = temp::TempFactory::NewTemp();
    temp_map_->Enter(reg, new std::string("%" + reg_name));
    regs_.push_back(reg);
  }
}

tree::Exp *ExternalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}

tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  /* TODO: Put your lab5 code here */
  return nullptr;
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  /* TODO: Put your lab5 code here */
  body->Append(new assem::OperInstr("", new temp::TempList(),
                                    reg_manager->ReturnSink(), nullptr));
  return body;
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  /* TODO: Put your lab5 code here */

  return nullptr;
}

Access *Access::AllocLocal(Frame *frame, bool escape) {
  if (escape) {
    //  Frame::AllocLocal(TRUE) Returns an InFrameAccess with an offset from the
    //  frame pointer
    return new frame::InFrameAccess(frame->AllocLocal());
  } else {
    //  Frame::AllocLocal(FALSE) Returns a register InRegAccess(t481)
    return new frame::InRegAccess(temp::TempFactory::NewTemp());
  }
}

tree::Exp *InFrameAccess::ToExp(tree::Exp *framePtr) const {
  // return off(fp) (for visiting var on stack)
  return new tree::MemExp(new tree::BinopExp(
      tree::BinOp::PLUS_OP, new tree::ConstExp(offset), framePtr));
}

tree::Exp *InRegAccess::ToExp(tree::Exp *framePtr) const {
  // visiting var in reg
  return new tree::TempExp(reg);
}
} // namespace frame
