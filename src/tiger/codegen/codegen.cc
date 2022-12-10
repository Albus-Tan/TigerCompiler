#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {

void CodeGen::Codegen() {
  fs_ = frame_->GetLabel() + "_framesize"; // // Frame size label_
  auto instr_list = new assem::InstrList();

  //  // Save callee-saved registers
  //  auto pos = reg_manager->GetRegister(frame::X64RegManager::X64Reg::RAX);
  //  instr_list->Append(new assem::OperInstr("leaq " + fs_ + "(%rsp), `d0",
  //                                          new temp::TempList(pos), nullptr,
  //                                          nullptr));
  //  instr_list->Append(
  //      new assem::OperInstr("addq $" + std::to_string(frame_->offset_) + ",
  //      `d0",
  //                           new temp::TempList(pos), nullptr, nullptr));
  //  for (auto callee_save_reg : reg_manager->CalleeSaves()->GetList()) {
  //    PushRegToPos(*instr_list, pos, callee_save_reg);
  //  }

  // Init FP with SP
  // FP = SP + fs
  //  instr_list->Append(new assem::OperInstr(
  //      "leaq " + fs_ + "(`s0), `d0",
  //      new temp::TempList(reg_manager->FramePointer()),
  //      new temp::TempList(reg_manager->StackPointer()), nullptr));

  // Munch
  for (auto stm : traces_->GetStmList()->GetList()) {
    stm->Munch(*instr_list, fs_);
  }

  //  // Restore callee-saved registers
  //  auto pos_rbx =
  //  reg_manager->GetRegister(frame::X64RegManager::X64Reg::RBX); auto li =
  //  reg_manager->CalleeSaves()->GetList(); instr_list->Append(new
  //  assem::OperInstr("leaq " + fs_ + "(%rsp), `d0",
  //                                          new temp::TempList(pos_rbx),
  //                                          nullptr, nullptr));
  //  instr_list->Append(new assem::OperInstr(
  //      "addq $" +
  //          std::to_string(frame_->offset_ +
  //                         li.size() * reg_manager->WordSize()) +
  //          ", `d0",
  //      new temp::TempList(pos_rbx), nullptr, nullptr));
  //  for (auto callee_save_reg : li) {
  //    PopRegFromPos(*instr_list, pos_rbx, callee_save_reg);
  //  }

  assem_instr_ =
      std::make_unique<AssemInstr>(frame::ProcEntryExit2(instr_list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}

temp::TempList *MunchOperand(tree::Exp *exp, OperandRole role,
                             std::string &assem, assem::InstrList &instr_list,
                             std::string_view fs) {
  if (typeid(*exp) == typeid(tree::ConstExp)) {
    // imm
    assem =
        "$" + std::to_string(static_cast<const tree::ConstExp *>(exp)->consti_);
    return new temp::TempList();
  }
  if (typeid(*exp) == typeid(tree::MemExp)) {
    // mem
    assem = role == SRC ? "(`s0)" : "(`d0)";
    tree::Exp *addr = static_cast<const tree::MemExp *>(exp)->exp_;
    if (typeid(*addr) == typeid(tree::BinopExp)) {
      // check if format of address is reg + const
      tree::BinopExp *binop_exp = static_cast<tree::BinopExp *>(addr);
      tree::Exp *left = binop_exp->left_, *right = binop_exp->right_;
      // IMM(%REG)
      if (typeid(*left) == typeid(tree::ConstExp)) {
        int offset = static_cast<const tree::ConstExp *>(left)->consti_;
        assem = std::to_string(offset) + assem;
        return new temp::TempList(right->Munch(instr_list, fs));
      }
      if (typeid(*right) == typeid(tree::ConstExp)) {
        int offset = static_cast<const tree::ConstExp *>(right)->consti_;
        assem = std::to_string(offset) + assem;
        return new temp::TempList(left->Munch(instr_list, fs));
      }
    }
    return new temp::TempList(addr->Munch(instr_list, fs));
  }
  // reg
  assem = role == SRC ? "`s0" : "`d0";
  return new temp::TempList(exp->Munch(instr_list, fs));
}
} // namespace cg

namespace tree {

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  instr_list.Append(
      new assem::LabelInstr(temp::LabelFactory::LabelString(label_), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto dst_label = exp_->name_->Name();
  assem::Instr *instr = new assem::OperInstr(
      "jmp " + dst_label, nullptr, nullptr, new assem::Targets(jumps_));
  instr_list.Append(instr);
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);
  // s0 - s1 (left - right)
  instr_list.Append(new assem::OperInstr(
      "cmpq `s1, `s0", nullptr, new temp::TempList({left, right}), nullptr));
  std::string cjump_instr;
  switch (op_) {
  case EQ_OP:
    cjump_instr = "je";
    break;
  case NE_OP:
    cjump_instr = "jne";
    break;
  case LT_OP:
    cjump_instr = "jl";
    break;
  case GT_OP:
    cjump_instr = "jg";
    break;
  case LE_OP:
    cjump_instr = "jle";
    break;
  case GE_OP:
    cjump_instr = "jge";
    break;
  case ULT_OP:
    cjump_instr = "jnb";
    break;
  case ULE_OP:
    cjump_instr = "jnbe";
    break;
  case UGT_OP:
    cjump_instr = "jna";
    break;
  case UGE_OP:
    cjump_instr = "jnae";
    break;
  case REL_OPER_COUNT:
  default:
    assert(0);
  }
  instr_list.Append(
      new assem::OperInstr(cjump_instr + " `j0", nullptr, nullptr,
                           new assem::Targets(new std::vector<temp::Label *>{
                               true_label_, false_label_})));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *src = src_->Munch(instr_list, fs);
  if (typeid(*dst_) == typeid(tree::MemExp)) {
    // deal with dst is mem
    // directly move to mem
    temp::Temp *dst = ((MemExp *)dst_)->exp_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(
        "movq `s0, (`s1)", nullptr, new temp::TempList({src, dst}), nullptr));
  } else {
    // dst is reg
    // if src is mem exp, src_->Munch will deal with this
    temp::Temp *dst = dst_->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList({dst}), new temp::TempList({src})));
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *result = nullptr;
  std::string op_instr;
  switch (op_) {
  case PLUS_OP:
    op_instr = "addq";
    break;
  case MINUS_OP:
    op_instr = "subq";
    break;
  case MUL_OP:
    op_instr = "imulq";
    break;
  case DIV_OP:
    op_instr = "idivq";
    break;
  case AND_OP:
    op_instr = "andq";
    break;
  case OR_OP:
    op_instr = "orq";
    break;
  case LSHIFT_OP:
    op_instr = "salq";
    break;
  case RSHIFT_OP:
    op_instr = "shrq";
    break;
  case ARSHIFT_OP:
    op_instr = "sarq";
    break;
  case XOR_OP:
    op_instr = "xorq";
    break;
  case BIN_OPER_COUNT:
  default:
    assert(0);
  }

  temp::Temp *rax = reg_manager->GetRegister(frame::X64RegManager::X64Reg::RAX);
  temp::Temp *rdx = reg_manager->GetRegister(frame::X64RegManager::X64Reg::RDX);

  if (op_ == BinOp::DIV_OP) {
    temp::Temp *reg = temp::TempFactory::NewTemp();

    // idivq S
    // R[%rdx] <- R[%rdx]:R[%rax] mod S
    // R[%rax] <- R[%rdx]:R[%rax] / S
    instr_list.Append(
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rax),
                             new temp::TempList(left_->Munch(instr_list, fs))));
    instr_list.Append(new assem::OperInstr("cqto",
                                           new temp::TempList({rax, rdx}),
                                           new temp::TempList(rax), nullptr));
    instr_list.Append(new assem::OperInstr(
        "idivq `s0", new temp::TempList({rax, rdx}),
        new temp::TempList(right_->Munch(instr_list, fs)), nullptr));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(reg), new temp::TempList(rax)));
    return reg;
  }
  if (op_ == BinOp::MUL_OP) {
    temp::Temp *reg = temp::TempFactory::NewTemp();
    // imulq S
    // R[%rdx]:R[%rax] <- S * R[%rax]
    instr_list.Append(
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rax),
                             new temp::TempList(left_->Munch(instr_list, fs))));
    instr_list.Append(new assem::OperInstr(
        "imulq `s0", new temp::TempList({rax, rdx}),
        new temp::TempList(right_->Munch(instr_list, fs)), nullptr));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(reg), new temp::TempList(rax)));
    return reg;
  }

  std::stringstream assem;
  std::string leftAssem, rightAssem;
  temp::TempList *left =
      cg::MunchOperand(left_, cg::OperandRole::SRC, leftAssem, instr_list, fs);
  temp::TempList *right = cg::MunchOperand(right_, cg::OperandRole::SRC,
                                           rightAssem, instr_list, fs);
  result = temp::TempFactory::NewTemp();

  // Move an operand into register
  // Destination is also used as a operand
  right->Append(result);
  temp::TempList *dst = new temp::TempList(result);
  assem << "movq " << leftAssem << ", `d0";
  if (typeid(*left_) == typeid(tree::MemExp)) {
    instr_list.Append(new assem::OperInstr(assem.str(), dst, left, nullptr));
  } else {
    instr_list.Append(new assem::MoveInstr(assem.str(), dst, left));
  }
  assem.str("");
  assem << op_instr << ' ' << rightAssem << ", `d0";
  instr_list.Append(new assem::OperInstr(assem.str(), dst, right, nullptr));
  return result;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  // only deal with src is mem
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::Temp *exp = exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0",
                                         new temp::TempList(reg),
                                         new temp::TempList(exp), nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if (temp_ != reg_manager->FramePointer()) {
    return temp_;
  }

  // modify FP to SP + frame_size since we do not have FP
  temp::Temp *fp = temp::TempFactory::NewTemp();
  std::stringstream assem;
  assem << "leaq " << fs << "(`s0), `d0";
  instr_list.Append(new assem::OperInstr(
      assem.str(), new temp::TempList(fp),
      new temp::TempList(reg_manager->StackPointer()), nullptr));
  return fp;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *dst = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::OperInstr("leaq " + name_->Name() + "(%rip), `d0",
                           new temp::TempList(dst), nullptr, nullptr));
  return dst;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *dst = temp::TempFactory::NewTemp();
  // Imm
  instr_list.Append(
      new assem::OperInstr("movq $" + std::to_string(consti_) + ", `d0",
                           new temp::TempList(dst), nullptr, nullptr));
  return dst;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // prepare arguments
  // should be listed as “sources” of the instruction
  temp::TempList *arg_regs = args_->MunchArgs(instr_list, fs);

  // CALL instruction will trash caller saved registers
  // specifies these registers and as “destinations” of the call
  instr_list.Append(new assem::OperInstr(
      "callq " + (static_cast<NameExp *>(fun_))->name_->Name(),
      reg_manager->CallerSaves(), arg_regs, nullptr));
  // get return value
  temp::Temp *return_temp = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::MoveInstr("movq `s0, `d0", new temp::TempList(return_temp),
                           new temp::TempList(reg_manager->ReturnValue())));
  // reset sp when existing arguments passed on the stack
  int arg_reg_num = reg_manager->ArgRegs()->GetList().size();
  int arg_stack_num = std::max(int(args_->GetList().size()) - arg_reg_num, 0);
  if (arg_stack_num > 0) {
    instr_list.Append(new assem::OperInstr(
        "addq $" + std::to_string(arg_stack_num * reg_manager->WordSize()) +
            ", `d0",
        new temp::TempList(reg_manager->StackPointer()), nullptr, nullptr));
  }
  return return_temp;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  // ExpList::MunchArgs generates code to move all the arguments to their
  // correct position
  // MunchArgs(il) will iterate exp_list for all arguments , generate
  // corresponding assem to move them all
  temp::TempList *arg_regs = new temp::TempList();
  int arg_idx = 0;
  int arg_reg_num = reg_manager->ArgRegs()->GetList().size();
  for (Exp *exp : exp_list_) {
    temp::Temp *src = exp->Munch(instr_list, fs);
    if (arg_idx < arg_reg_num) {
      // pass in reg
      temp::Temp *reg = reg_manager->ArgRegs()->NthTemp(arg_idx);
      instr_list.Append(new assem::MoveInstr(
          "movq `s0, `d0", new temp::TempList(reg), new temp::TempList(src)));
      arg_regs->Append(reg);
    } else {
      // pass on stack
      // pushq (subq wordsize fo SP, then mov to SP)
      instr_list.Append(new assem::OperInstr(
          "subq $" + std::to_string(reg_manager->WordSize()) + ", `d0",
          new temp::TempList(reg_manager->StackPointer()), nullptr, nullptr));
      instr_list.Append(new assem::OperInstr(
          "movq `s0, (`d0)", new temp::TempList(reg_manager->StackPointer()),
          new temp::TempList(src), nullptr));
    }
    ++arg_idx;
  }
  // returns a list of all the temporaries that are to be passed to the
  // machine’s call instructions
  return arg_regs;
}

} // namespace tree
