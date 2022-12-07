#include <sstream>
#include "tiger/frame/x64frame.h"
#include "frame.h"

extern frame::RegManager *reg_manager;

namespace frame {

int X64Frame::AllocLocal() {
  // Keep away from the return address on the top of the frame
  offset_ -= reg_manager->WordSize();
  return offset_;
}

X64Frame::X64Frame(temp::Label *name, std::list<bool> formals) : Frame(name) {
  formals_ = new std::list<frame::Access *>();
  for (auto formal_escape : formals) {
    formals_->push_back(frame::Access::AllocLocal(this, formal_escape));
  }
}

std::list<frame::Access *> *X64Frame::Formals() { return nullptr; }

/* TODO: Put your lab5 code here */
temp::TempList *X64RegManager::Registers() {
  /* TODO: Put your lab5 code here */
  // !!!typo!!! should be RSP not RSI
  // except rsp
  /**
   * Get general-purpose registers except RSP
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  return new temp::TempList({
      regs_[RAX],
      regs_[RBX],
      regs_[RCX],
      regs_[RDX],
      regs_[RDI],
      regs_[RBP],
      regs_[RSI],
      regs_[R8],
      regs_[R9],
      regs_[R10],
      regs_[R11],
      regs_[R12],
      regs_[R13],
      regs_[R14],
      regs_[R15],
  });
}

temp::TempList *X64RegManager::ArgRegs() {
  /* TODO: Put your lab5 code here */
  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  return new temp::TempList({
      regs_[RDI],
      regs_[RSI],
      regs_[RDX],
      regs_[RCX],
      regs_[R8],
      regs_[R9],
  });
}

temp::TempList *X64RegManager::CallerSaves() {
  /* TODO: Put your lab5 code here */
  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  return new temp::TempList({
      regs_[R10],
      regs_[R11],
  });
}

temp::TempList *X64RegManager::CalleeSaves() {
  /* TODO: Put your lab5 code here */
  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  return new temp::TempList({
      regs_[RBX],
      regs_[RBP],
      regs_[R12],
      regs_[R13],
      regs_[R14],
      regs_[R15],
  });
}

temp::TempList *X64RegManager::ReturnSink() {
  /* TODO: Put your lab5 code here */
  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  temp::TempList *temp_list = CalleeSaves();
  temp_list->Append(StackPointer());
  temp_list->Append(ReturnValue());
  return temp_list;
}

int X64RegManager::WordSize() {
  /* TODO: Put your lab5 code here */
  return WORD_SIZE;
}

temp::Temp *X64RegManager::FramePointer() {
  /* TODO: Put your lab5 code here */
  return regs_[RBP];
}

temp::Temp *X64RegManager::StackPointer() {
  /* TODO: Put your lab5 code here */
  return regs_[RSP];
}

temp::Temp *X64RegManager::ReturnValue() {
  /* TODO: Put your lab5 code here */
  return regs_[RAX];
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
  // TODO: may have bugs
  // num of regs that can store arg
  auto arg_reg_num = reg_manager->ArgRegs()->GetList().size();
  // num of arg of proc
  auto arg_num = frame->formals_->size();
  int formal_idx = 0;  // current processing formal index
  tree::SeqStm *view_shift = nullptr, *tail = nullptr;
  for (Access *formal : *(frame->formals_)) {
    tree::Exp *dst =
        formal->ToExp(new tree::TempExp(reg_manager->FramePointer()));
    tree::Exp *src;
    if (formal_idx < arg_reg_num) {
      // in reg
      src = new tree::TempExp(reg_manager->ArgRegs()->NthTemp(formal_idx));
    } else {
      // in stack
      // TODO: may have bugs in offset
      src = new tree::MemExp(new tree::BinopExp(
          tree::BinOp::PLUS_OP, new tree::TempExp(reg_manager->FramePointer()),
          new tree::ConstExp((arg_num - formal_idx) *
                             reg_manager->WordSize())));
    }
    tree::MoveStm *move_stm = new tree::MoveStm(dst, src);
    if (!tail) {
      view_shift = tail = new tree::SeqStm(move_stm, nullptr);
    } else {
      tail->right_ = new tree::SeqStm(move_stm, nullptr);
      tail = static_cast<tree::SeqStm *>(tail->right_);
    }
    ++formal_idx;
  }
  if (view_shift) {
    tail->right_ = stm;
    return view_shift;
  }
  return stm;
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  /* TODO: Put your lab5 code here */
  body->Append(new assem::OperInstr("", new temp::TempList(),
                                    reg_manager->ReturnSink(), nullptr));
  return body;
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  /* TODO: Put your lab5 code here */
  // TODO: may have bugs

  // prolog part
  std::stringstream prologue;
  const std::string name = temp::LabelFactory::LabelString(frame->name_);
  const int rsp_offset = frame->Size();
  prologue << ".set " << name << "_framesize, " << rsp_offset << std::endl;
  prologue << name << ":" << std::endl;
  prologue << "subq $" << rsp_offset << ", %rsp" << std::endl;

  // epilog part
  std::stringstream epilogue;
  epilogue << "addq $" << rsp_offset << ", %rsp" << std::endl;
  epilogue << "retq" << std::endl << ".END" << std::endl;
  return new assem::Proc(prologue.str(), body, epilogue.str());
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
