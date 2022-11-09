#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
};

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

tree::Exp *ExternalCall(std::string s, tree::ExpList *args){
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
}

} // namespace frame