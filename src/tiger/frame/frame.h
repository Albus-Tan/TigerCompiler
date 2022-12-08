#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"


namespace frame {

class Access;
class Frame;

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  // FIXME: TYPO!!!!! should be RSP
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

// var location in frame or reg
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
public:
  // formals_ extracts a list of k “accesses”
  // denoting the locations where the formal parameters will be kept at run time
  // as seen from inside the callee
  // first is static link
  std::list<frame::Access *> *formals_; // The locations of all the formals ()
  int offset_ = 0;                        // - frame size

  // label at which the function’s machine code is to begin
  temp::Label *name_ = nullptr; // indicate the return address (jump to according label)
public:
  Frame() {}
  Frame(temp::Label *name) : offset_(0), name_(name) {}
  ~Frame() {}
  [[nodiscard]] virtual int Size() { return -offset_; }
  [[nodiscard]] std::string GetLabel() { return name_->Name(); }
  [[nodiscard]] std::list<frame::Access *> *GetFormals() {return formals_;}
  virtual int AllocLocal() = 0;  // return an offset from the frame pointer
};



/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag *> frags_;
};

/* TODO: Put your lab5 code here */
tree::Exp *ExternalCall(std::string s, tree::ExpList *args);
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm);
assem::InstrList *ProcEntryExit2(assem::InstrList *body);
assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body);

} // namespace frame

#endif