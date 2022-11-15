#include "tiger/frame/x64frame.h"
#include "frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *frame_ptr) const override;
};

class InRegAccess : public Access {
public:
  temp::Temp *reg; // Temp is a data structure represents virtual registers
  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  int AllocLocal();
  std::list<frame::Access *> *Formals();
};

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

} // namespace frame
