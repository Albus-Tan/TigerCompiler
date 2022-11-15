#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace frame {

class Access;
class Frame;

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;

protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  Access() {}
  virtual tree::Exp *ToExp(tree::Exp *framePtr)
      const = 0; // Get the expression to access the variable
  static Access *AllocLocal(Frame *frame, bool escape);

  virtual ~Access() = default;
};

class Frame {
  /* TODO: Put your lab5 code here */
protected:
  // formals_ extracts a list of k “accesses”
  // denoting the locations where the formal parameters will be kept at run time
  // as seen from inside the callee
  std::list<frame::Access *> *formals_; // The locations of all the formals
  int offset_ = 0;                        // - frame size

  // label at which the function’s machine code is to begin
  temp::Label *label_ =
      nullptr; // indicate the return address (jump to according label)
public:
  Frame() {}
  Frame(temp::Label *label, int size = 0) : label_(label), offset_(-size) {}
  ~Frame() {}
  [[nodiscard]] int Size() const { return -offset_; }
  [[nodiscard]] temp::Label *GetLabel() { return label_; }
  [[nodiscard]] std::list<frame::Access *> *GetFormals() {return formals_;}
  virtual int AllocLocal() = 0;  // return an offset from the frame pointer
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.push_back(frag); }
  const std::list<Frag *> &GetList() { return frags_; }

private:
  std::list<Frag *> frags_;
};

/* TODO: Put your lab5 code here */
tree::Exp *ExternalCall(std::string s, tree::ExpList *args);

} // namespace frame

#endif