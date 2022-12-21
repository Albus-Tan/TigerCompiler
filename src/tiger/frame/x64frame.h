//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

//#define DEBUG_FRAME

#ifdef DEBUG_FRAME
#define DBG_FRAME(format, ...) fprintf(stderr, \
"[DEBUG_FRAME](%s, %s(), Line %d): " \
, __FILE__, __FUNCTION__, __LINE__);     \
fprintf(stdout, format"\r\n", ##__VA_ARGS__)
#else
#define DBG_FRAME(format, ...)  do {} while (0)
#endif

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  enum X64Reg {
    RBX = 0,
    RCX,
    RDX,
    RSI,
    RDI,
    RBP,
    RSP,
    RAX,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
  };

  const std::string X64RegNames[16] = {"rbx", "rcx", "rdx", "rsi", "rdi", "rbp",
                                       "rsp", "rax", "r8",  "r9",  "r10", "r11",
                                       "r12", "r13", "r14", "r15"};
  const int WORD_SIZE = 8;

public:
  X64RegManager();

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  temp::TempList *Registers();

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  temp::TempList *ArgRegs();

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  temp::TempList *CallerSaves();

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  temp::TempList *CalleeSaves();

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  temp::TempList *ReturnSink();

  /**
   * Get word size
   */
  int WordSize();

  temp::Temp *FramePointer();

  temp::Temp *StackPointer();

  temp::Temp *ReturnValue();
};

/* TODO: Put your lab5 code here */
// visiting var in frame
class InFrameAccess : public Access {
public:
  int offset;
  bool store_pointer;
  explicit InFrameAccess(int offset, bool store_pointer)
      : offset(offset), store_pointer(store_pointer) {}
  /* TODO: Put your lab5 code here */

  // return off(fp) (for visiting var on stack)
  tree::Exp *ToExp(tree::Exp *framePtr) const override;
  void SetStorePointer(bool store_pointer_) override;
};

// visiting var in reg
class InRegAccess : public Access {
public:
  temp::Temp *reg; // Temp is a data structure represents virtual registers
  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override;
  void SetStorePointer(bool store_pointer) override;
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label *name, std::list<bool> formals, std::list<bool> store_pointer);
  int AllocLocal();
  std::list<frame::Access *> *Formals();
  int Size() override;
  std::vector<int> GetPointerOffsets() override;
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
