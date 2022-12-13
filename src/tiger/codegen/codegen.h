#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/canon/canon.h"
#include "tiger/codegen/assem.h"
#include "tiger/frame/x64frame.h"
#include "tiger/translate/tree.h"

// Forward Declarations
namespace frame {
class RegManager;
class Frame;
} // namespace frame

namespace assem {
class Instr;
class InstrList;
} // namespace assem

namespace canon {
class Traces;
} // namespace canon

namespace cg {

enum OperandRole { SRC, DST };
/**
 * Select suitable addressing mode for a memory address
 * @param assem Output assembly
 * @return List of registers used in assem
 */
temp::TempList *MunchMemAddr(tree::Exp *addr, OperandRole role,
                             std::string &assem, assem::InstrList &instr_list,
                             std::string_view fs);
/**
 * Select suitable addressing mode for an operand
 * @param assem Output assembly
 * @return List of registers used in assem
 */
temp::TempList *MunchOperand(tree::Exp *exp, OperandRole role,
                             std::string &assem, assem::InstrList &instr_list,
                             std::string_view fs);

class AssemInstr {
public:
  AssemInstr() = delete;
  AssemInstr(nullptr_t) = delete;
  explicit AssemInstr(assem::InstrList *instr_list) : instr_list_(instr_list) {}

  void Print(FILE *out, temp::Map *map) const;

  [[nodiscard]] assem::InstrList *GetInstrList() const { return instr_list_; }

private:
  assem::InstrList *instr_list_;
};

class CodeGen {
public:
  CodeGen(frame::Frame *frame, std::unique_ptr<canon::Traces> traces)
      : frame_(frame), traces_(std::move(traces)) {}

  void Codegen();
  std::unique_ptr<AssemInstr> TransferAssemInstr() {
    return std::move(assem_instr_);
  }

private:
  frame::Frame *frame_;
  std::string fs_; // Frame size label_
  std::unique_ptr<canon::Traces> traces_;
  std::unique_ptr<AssemInstr> assem_instr_;
  void PushRegToPos(assem::InstrList &instr_list, temp::Temp *pos,
                    temp::Temp *to_be_push);
  void PopRegFromStack(assem::InstrList &instr_list, temp::Temp *reg);
  void PushRegOnStack(assem::InstrList &instr_list, temp::Temp *reg);
  void PopRegFromPos(assem::InstrList &instr_list, temp::Temp *pos,
                     temp::Temp *to_be_pop);
};

} // namespace cg
#endif
