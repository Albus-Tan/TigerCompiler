#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    tree::CjumpStm *stm = new tree::CjumpStm(
        tree::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    std::list<temp::Label **> true_patch_list{&stm->true_label_};
    std::list<temp::Label **> false_patch_list{&stm->false_label_};
    return {PatchList(true_patch_list), PatchList(false_patch_list), stm};
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    // For UnCX should never expect to see a tr::NxExp kind
    assert(0);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
                new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                                                    new tree::ConstExp(0)),
                                  new tree::EseqExp(new tree::LabelStm(t),
                                                    new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  FillBaseTEnv();
  FillBaseVEnv();
  /* TODO: Put your lab5 code here */
}

// TODO may have bugs
// void and error return type
// exp (new tr::ExExp(new tree::ConstExp(0)))
// stm (new tree::ExpStm(new tree::ConstExp(0)))
static tr::ExExp *getVoidExp() { return new tr::ExExp(new tree::ConstExp(0)); }

static tr::ExpAndTy *getVoidExpAndNilTy() {
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

static tree::ExpStm *getVoidStm() {
  return new tree::ExpStm(new tree::ConstExp(0));
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    env::VarEntry *var = static_cast<env::VarEntry *>(entry);
    // TODO
    // Suppose variable is x and its acc is inFrame
    // If access is in the level, Resulting tree is MEM(+(CONST(k), TEMP(FP)))
    // Otherwise, static links must be used
    // MEM( + ( CONST k, MEM(( …MEM(TEMP FP) …))))
    // n static links must be follow, k is the offset of x in the defined
    // function

    // return var->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return tr::getVoidExpAndNilTy();
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // check if var (lvalue) is record
  tr::ExpAndTy *exp_and_ty =
      var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *type = exp_and_ty->ty_->ActualTy();
  if (typeid(*type) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return tr::getVoidExpAndNilTy();
  }

  // check if record has this field
  type::RecordTy *record = static_cast<type::RecordTy *>(type);
  for (type::Field *field : record->fields_->GetList()) {
    if (field->name_->Name() == sym_->Name()) {
      // A.f is MEM(+(MEM(e), CONST offset f))
      // Add the constant field offset of f to the address A
      tree::MemExp *exp = new tree::MemExp(
          new tree::BinopExp(tree::BinOp::PLUS_OP, exp_and_ty->exp_->UnEx(),
                             new tree::ConstExp(reg_manager->WordSize())));
      return new tr::ExpAndTy(new tr::ExExp(exp), field->ty_);
    }
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return tr::getVoidExpAndNilTy();
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *subscript =
      subscript_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *var_type = var->ty_;
  if (typeid(*var_type) == typeid(type::ArrayTy)) {
    // a[i] is MEM(+(MEM(e), *(i, CONST w))
    tree::MemExp *exp = new tree::MemExp(new tree::BinopExp(
        tree::BinOp::PLUS_OP, var->exp_->UnEx(),
        new tree::BinopExp(tree::BinOp::MUL_OP, subscript->exp_->UnEx(),
                           new tree::ConstExp(reg_manager->WordSize()))));
    return new tr::ExpAndTy(
        new tr::ExExp(exp),
        (static_cast<type::ArrayTy *>(var_type)->ty_->ActualTy()));
  } else {
    errormsg->Error(pos_, "array type required");
    return tr::getVoidExpAndNilTy();
  }
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // Make a new label lab
  temp::Label *lab = temp::LabelFactory::NewLabel();
  // Puts the assembly language fragment frame::StringFrag(lab, lit) onto a
  // global list
  frags->PushBack(new frame::StringFrag(lab, str_));
  // Returns the tree tree::NameExp(lab)
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(lab)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right = right_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *left_ty = left->ty_;
  type::Ty *right_ty = right->ty_;

  if (oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP ||
      oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP) {
    // check if operands of arithmetic operators are integers
    if (typeid(*left_ty) != typeid(type::IntTy)) {
      errormsg->Error(left_->pos_, "integer required");
      return new tr::ExpAndTy(tr::getVoidExp(), type::IntTy::Instance());
    }
    if (typeid(*right_ty) != typeid(type::IntTy)) {
      errormsg->Error(right_->pos_, "integer required");
      return new tr::ExpAndTy(tr::getVoidExp(), type::IntTy::Instance());
    }

    return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp(
                                static_cast<tree::BinOp>(oper_),
                                left->exp_->UnEx(), right->exp_->UnEx())),
                            type::IntTy::Instance());
  } else {
    if (!left_ty->IsSameType(right_ty)) {
      // check if operands type is same
      errormsg->Error(pos_, "same type required");
      return left;
    }
    /* TODO: Put your lab5 code here */

    //    return type::IntTy::Instance();
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {

  // check if record type is defined
  type::Ty *type = tenv->Look(typ_);
  if (type && typeid(*(type->ActualTy())) == typeid(type::RecordTy)) {
    type::RecordTy *recordTy = static_cast<type::RecordTy *>(type->ActualTy());

    // Record creation and initialization
    // { f1 = e1 ; f2 = e2; … ; fn = en }

    // A record may outlive the procedure activation that creates it
    // cannot be allocated on the stack, must be allocated on the heap

    // returns the pointer into a new temporary r
    temp::Temp *r = temp::TempFactory::NewTemp();
    tree::ExpList *args = new tree::ExpList();
    // creates an n-word area (CONST n*w)
    args->Append(new tree::ConstExp((recordTy->fields_->GetList().size()) *
                                    reg_manager->WordSize()));
    // Call an external memory-allocation function
    // runtime.c: int *alloc_record(int size)
    tree::MoveStm *left_move_stm = new tree::MoveStm(
        new tree::TempExp(r), frame::ExternalCall("alloc_record", args));

    tree::SeqStm *initialization_list_head = nullptr;
    tree::SeqStm *left_init_seq_stm =
        new tree::SeqStm(left_move_stm, initialization_list_head);
    tree::SeqStm *last_seq_stm = left_init_seq_stm;

    // check if num of fields in RecordExp is same as in definition
    if (fields_->GetList().size() != recordTy->fields_->GetList().size()) {
      errormsg->Error(pos_, "num of field doesn't match");
    }

    // check if each field in RecordExp is same as in definition
    auto record_field = recordTy->fields_->GetList().cbegin();
    type::FieldList *fieldList = new type::FieldList();
    int idx = 0;
    for (absyn::EField *actual_field : fields_->GetList()) {

      auto actual_field_exp_and_ty =
          actual_field->exp_->Translate(venv, tenv, level, label, errormsg);
      fieldList->Append(
          new type::Field(actual_field->name_, actual_field_exp_and_ty->ty_));

      // check field name
      if (actual_field->name_->Name() != (*record_field)->name_->Name()) {
        errormsg->Error(pos_, "field %s doesn't exist",
                        actual_field->name_->Name().data());
      }
      // check field type
      if (!actual_field_exp_and_ty->ty_->IsSameType((*record_field)->ty_)) {
        errormsg->Error(pos_, "type of field %s doesn't match",
                        actual_field->name_->Name().data());
      }

      // A series of MOVE trees can initialize offsets 0, 1w, 2w, …,(n-1)w
      // from r with the translations of expression ei
      tree::BinopExp *address_exp =
          new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(r),
                             new tree::ConstExp(idx * reg_manager->WordSize()));
      tree::MoveStm *move_stm = new tree::MoveStm(
          new tree::MemExp(address_exp), actual_field_exp_and_ty->exp_->UnEx());
      tree::SeqStm *seq_stm = new tree::SeqStm(move_stm, nullptr);
      last_seq_stm->right_ = seq_stm;
      last_seq_stm = seq_stm;

      ++record_field;
      ++idx;
    }
    // fill the last one with void stm for simple
    last_seq_stm->right_ = tr::getVoidStm();
    // top ESEQ
    tree::EseqExp *eseq_exp =
        new tree::EseqExp(left_init_seq_stm, new tree::TempExp(r));
    // empty record
    if (fields_->GetList().empty()) {
      return new tr::ExpAndTy(new tr::ExExp(eseq_exp), type::NilTy::Instance());
    }
    // The result of the whole expression is r
    return new tr::ExpAndTy(new tr::ExExp(eseq_exp), type->ActualTy());
  } else {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return tr::getVoidExpAndNilTy();
  }
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *varTy = var->ty_->ActualTy();
  type::Ty *expTy = exp->ty_->ActualTy();

  // check if var and exp has the same type
  if (varTy->IsSameType(expTy)) {

    // check if loop variable (readonly) is assigned
    if (typeid(*var_) == typeid(absyn::SimpleVar)) {
      absyn::SimpleVar *var = static_cast<absyn::SimpleVar *>(var_);
      env::VarEntry *entry =
          static_cast<env::VarEntry *>(venv->Look(var->sym_));
      if (entry->readonly_) {
        errormsg->Error(pos_, "loop variable can't be assigned");
      }
    }

  } else {
    errormsg->Error(pos_, "unmatched assign exp");
  }
  tree::MoveStm *stm = new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx());
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // If e1 then e2 else e3
  // Treat e1 as a Cx (apply unCx to e1)
  // String comparison
  // For string equal just calls runtime –system function stringEqual
  // For string unequal just calls runtime –system function stringEqual then
  // complements the result
  // Treat e2 and e3 as Ex (apply unEx to e2 and e3)
  // If e2 and e3 are both “statements” (Nx), Translate the result as statement
  // If e2 or e3 is a Cx
  // Make two labels t and f to which the conditional will branch
  // Allocate a temporary r
  // after label t,  move e2 to r
  // after label f, move e3 to r
  // Both braches should finish by jumping newly  created “joint” label
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {

  /* TODO: Put your lab5 code here */

  temp::Label *doneLabel = temp::LabelFactory::NewLabel();

  tr::ExpAndTy *test_exp_and_ty =
      test_->Translate(venv, tenv, level, label, errormsg);
  // check if body of while have no value
  // if break in body, goto done label
  // Exp::Translate has a new formal parameter break
  // done label must be passed as the break parameter when translate body
  tr::ExpAndTy *body_exp_and_ty =
      body_->Translate(venv, tenv, level, doneLabel, errormsg);
  type::Ty *bodyTy = body_exp_and_ty->ty_;
  if (!bodyTy->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(pos_, "while body must produce no value");
  }

  temp::Label *testLabel = temp::LabelFactory::NewLabel();
  temp::Label *bodyLabel = temp::LabelFactory::NewLabel();
  tr::Cx test_cx = test_exp_and_ty->exp_->UnCx(errormsg);

  test_cx.trues_ = tr::PatchList({&bodyLabel});
  test_cx.falses_ = tr::PatchList({&doneLabel});

  //  test:
  //    if not(condition) goto done
  //  body:
  //    body
  //    goto test
  //  done:
  tree::SeqStm *seq_stm = new tree::SeqStm(
      new tree::LabelStm(testLabel),
      new tree::SeqStm(
          test_cx.stm_,
          new tree::SeqStm(
              new tree::LabelStm(bodyLabel),
              new tree::SeqStm(
                  body_exp_and_ty->exp_->UnNx(),
                  new tree::SeqStm(
                      new tree::JumpStm(
                          new tree::NameExp(testLabel),
                          new std::vector<temp::Label *>{testLabel}),
                      new tree::LabelStm(doneLabel))))));

  return new tr::ExpAndTy(new tr::NxExp(seq_stm), type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // check if array type is defined
  type::Ty *type = tenv->Look(typ_);
  if (type && typeid(*(type->ActualTy())) == typeid(type::ArrayTy)) {

    // check if size is int
    tr::ExpAndTy *size_exp_and_ty =
        size_->Translate(venv, tenv, level, label, errormsg);
    ;
    type::Ty *size_ty = size_exp_and_ty->ty_;
    if (typeid(*size_ty) != typeid(type::IntTy)) {
      errormsg->Error(pos_, "size of array should be int");
      return new tr::ExpAndTy(tr::getVoidExp(), type::VoidTy::Instance());
    }

    // check if type of init is same as array
    type::Ty *arrayTy = static_cast<type::ArrayTy *>(type->ActualTy())->ty_;
    tr::ExpAndTy *init_exp_and_ty =
        init_->Translate(venv, tenv, level, label, errormsg);
    type::Ty *initTy = init_exp_and_ty->ty_;
    if (!initTy->IsSameType(arrayTy)) {
      errormsg->Error(pos_, "type mismatch");
      return init_exp_and_ty;
    }

    tree::ExpList *args = new tree::ExpList();
    // runtime.c: long *init_array(int size, long init)
    args->Append(size_exp_and_ty->exp_->UnEx());
    args->Append(init_exp_and_ty->exp_->UnEx());
    temp::Temp *r = temp::TempFactory::NewTemp();
    // returns the pointer into a new temporary r
    // The result of the whole expression is r
    tree::EseqExp *exp = new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r),
                          frame::ExternalCall("init_array", args)),
        new tree::TempExp(r));
    return new tr::ExpAndTy(new tr::ExExp(exp), type->ActualTy());
  } else {
    errormsg->Error(pos_, "undefined array %s", typ_);
    return new tr::ExpAndTy(tr::getVoidExp(), type::VoidTy::Instance());
  }
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

} // namespace absyn
