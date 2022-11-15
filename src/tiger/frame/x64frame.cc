#include "tiger/frame/x64frame.h"
#include "frame.h"

extern frame::RegManager *reg_manager;

namespace frame {

int X64Frame::AllocLocal() {
  // Keep away from the return address on the top of the frame
  offset_ -= reg_manager->WordSize();
  return offset_;
}

/* TODO: Put your lab5 code here */
temp::TempList *X64RegManager::Registers() {
  /* TODO: Put your lab5 code here */
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
  return nullptr;
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
  return nullptr;
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  /* TODO: Put your lab5 code here */
  return nullptr;
}


Access *Access::AllocLocal(Frame *frame, bool escape) {

  return nullptr;
//  if (escape) {
//    //  Frame::AllocLocal(TRUE) Returns an InFrameAccess with an offset from the
//    //  frame pointer
//    return new frame::InFrameAccess(frame->AllocLocal());
//  } else {
//    //  Frame::AllocLocal(FALSE) Returns a register InRegAccess(t481)
//    return new frame::InRegAccess(temp::TempFactory::NewTemp());
//  }
}

} // namespace frame
