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

Access *Access::AllocLocal(Level *level, bool escape, bool store_pointer) {
  return new Access(
      level, frame::Access::AllocLocal(level->frame_, escape, store_pointer));
}

tree::Exp *Access::ToExp(Level *currentLevel) {
  // get the exp to access the variable, consider static links
  // get framePtr of the frame where var in
  tree::Exp *framePtr = currentLevel->StaticLink(level_);
  return access_->ToExp(framePtr);
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
    DBG("Start UnCx\n");
    /* TODO: Put your lab5 code here */
    tree::CjumpStm *stm = new tree::CjumpStm(
        tree::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    std::list<temp::Label **> true_patch_list{&(stm->true_label_)};
    std::list<temp::Label **> false_patch_list{&(stm->false_label_)};
    DBG("patch_list built");
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
    DBG("Start UnEx\n");
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    DBG("Start DoPatch");
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    DBG("Finish DoPatch");
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

  DBG("Start Translation\n");

  FillBaseTEnv();
  FillBaseVEnv();
  /* TODO: Put your lab5 code here */
  temp::Label *main_label_ = temp::LabelFactory::NamedLabel("tigermain");
  main_level_ =
      std::make_unique<Level>(nullptr, main_label_, std::list<bool>(), std::list<bool>());

  tr::ExpAndTy *main =
      absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(),
                             main_label_, errormsg_.get());
  frags->PushBack(new frame::ProcFrag(main->exp_->UnNx(), main_level_->frame_));
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

static tr::ExpAndTy *getVoidExpAndVoidTy() {
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::VoidTy::Instance());
}

static tree::ExpStm *getVoidStm() {
  return new tree::ExpStm(new tree::ConstExp(0));
}

// Static link: A frame pointer passed as the first parameter to callee
tree::Exp *Level::StaticLink(Level *targetLevel) {
  // Get the static link from current level to the target level
  Level *currentLevel = this;
  // get current frame pointer
  tree::Exp *framePtr = new tree::TempExp(reg_manager->FramePointer());
  // check from current level
  // target level must be current level ancestor
  while (currentLevel && currentLevel != targetLevel) {
    // get frame pointer out (first parameter)
    framePtr = currentLevel->frame_->formals_->front()->ToExp(framePtr);
    currentLevel = currentLevel->parent_;
  }
  // return framePtr of target level
  return framePtr;
}

std::list<Access *> *Level::Formals() {
  DBG("Step in");
  std::list<frame::Access *> *formal_list = frame_->formals_;
  DBG("std::list<frame::Access *> *formal_list get");
  std::list<tr::Access *> *formal_list_with_level =
      new std::list<tr::Access *>();
  DBG("std::list<tr::Access *> *formal_list_with_level newed");
  bool first = true;
  for (frame::Access *formal : *formal_list) {
    if (first) {
      // skip the first para static link
      DBG("skip the first para static link");
      first = false;
      continue;
    }
    DBG("formal_list_with_level push back para");
    formal_list_with_level->push_back(new tr::Access(this, formal));
  }
  DBG("std::list<tr::Access *> *formal_list_with_level built");
  return formal_list_with_level;
}

// tr::Level::NewLevel adds an extra element to the formal parameter list and
// calls formals.push_front(true); NewFrame(label, formals);
Level::Level(Level *parent, temp::Label *name, std::list<bool> formals, std::list<bool> store_pointers)
    : parent_(parent) {
  // add formal parameter for static link
  formals.push_front(true);
#ifdef GC
  store_pointers.push_front(false);
#endif
  // allocate new frame
  frame_ = new frame::X64Frame(name, formals, store_pointers);
}

} // namespace tr

namespace absyn {

#ifdef GC
// to check whether type is pointer (is record or array)
bool IsPointer(type::Ty *type) {
  return typeid(*(type->ActualTy())) == typeid(type::RecordTy) ||
         typeid(*(type->ActualTy())) == typeid(type::ArrayTy);
}
#endif

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  DBG("Start translate AbsynTree\n");
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  DBG("Start translate SimpleVar\n");

  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    env::VarEntry *var_entry = static_cast<env::VarEntry *>(entry);
    // Suppose variable is x and its acc is inFrame
    // If access is in the level, Resulting tree is MEM(+(CONST(k), TEMP(FP)))
    // Otherwise, static links must be used
    // MEM( + ( CONST k, MEM(( ???MEM(TEMP FP) ???))))
    // n static links must be follow, k is the offset of x in the defined
    // function
    tree::Exp *exp = var_entry->access_->ToExp(level);
    return new tr::ExpAndTy(new tr::ExExp(exp), var_entry->ty_->ActualTy());
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return tr::getVoidExpAndNilTy();
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  DBG("Start translate FieldVar\n");

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
  int idx = 0;
  for (type::Field *field : record->fields_->GetList()) {
    if (field->name_->Name() == sym_->Name()) {
      // A.f is MEM(+(MEM(e), CONST offset f))
      // Add the constant field offset of f to the address A
      tree::MemExp *exp = new tree::MemExp(new tree::BinopExp(
          tree::BinOp::PLUS_OP, exp_and_ty->exp_->UnEx(),
          new tree::ConstExp(idx * (reg_manager->WordSize()))));
      return new tr::ExpAndTy(new tr::ExExp(exp), field->ty_);
    }
    ++idx;
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return tr::getVoidExpAndNilTy();
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {

  DBG("Start translate SubscriptVar\n");

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
  DBG("Start translate VarExp\n");
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  DBG("Start translate NilExp\n");
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  DBG("Start translate IntExp\n");
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  DBG("Start translate StringExp\n");
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
  DBG("Start translate CallExp\n");
  /* TODO: Put your lab5 code here */
  // check if function is defined
  env::EnvEntry *entry = venv->Look(func_);
  if (entry && typeid(*entry) == typeid(env::FunEntry)) {
    env::FunEntry *func = static_cast<env::FunEntry *>(entry);

    // check if formals num match args num
    if (func->formals_->GetList().size() != args_->GetList().size()) {
      if (args_->GetList().size() > func->formals_->GetList().size()) {
        errormsg->Error(pos_ - 1,
                        "too many params in function " + func_->Name());
        return tr::getVoidExpAndVoidTy();
      } else {
        errormsg->Error(pos_ - 1, "para type mismatch");
        return tr::getVoidExpAndVoidTy();
      }
    }

    tree::ExpList *args_list = new tree::ExpList();
    // pass static link as the first parameter

    // If this is a user function
    if (func->level_) {
      // Pass static link as the first parameter
      args_list->Append(level->StaticLink(func->level_));
    }

    // check if types of actual and formal parameters match
    // need to use cbegin for const function
    auto formal_type = func->formals_->GetList().cbegin();
    for (Exp *arg : args_->GetList()) {
      tr::ExpAndTy *actual_exp_and_type =
          arg->Translate(venv, tenv, level, label, errormsg);
      type::Ty *actual_type = actual_exp_and_type->ty_;
      if (!actual_type->IsSameType(*formal_type)) {
        errormsg->Error(arg->pos_, "para type mismatch");
        return tr::getVoidExpAndVoidTy();
      }
      args_list->Append(actual_exp_and_type->exp_->UnEx());
      ++formal_type;
    }

    tree::CallExp *exp = new tree::CallExp(new tree::NameExp(func_), args_list);

    // return result type of function
    return new tr::ExpAndTy(new tr::ExExp(exp), func->result_);

  } else {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return tr::getVoidExpAndVoidTy();
  }
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  DBG("Start translate OpExp\n");
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

    tree::BinOp bin_op;
    switch (oper_) {
    case absyn::PLUS_OP:
      bin_op = tree::BinOp::PLUS_OP;
      break;
    case absyn::MINUS_OP:
      bin_op = tree::BinOp::MINUS_OP;
      break;
    case absyn::TIMES_OP:
      bin_op = tree::BinOp::MUL_OP;
      break;
    case absyn::DIVIDE_OP:
      bin_op = tree::BinOp::DIV_OP;
      break;
    }

    return new tr::ExpAndTy(
        new tr::ExExp(new tree::BinopExp(bin_op, left->exp_->UnEx(),
                                         right->exp_->UnEx())),
        type::IntTy::Instance());
  } else {
    if (!left_ty->IsSameType(right_ty)) {
      // check if operands type is same
      errormsg->Error(pos_, "same type required");
      return left;
    }

    if (oper_ == absyn::AND_OP || oper_ == absyn::OR_OP) {
      temp::Label *second_condition_label = temp::LabelFactory::NewLabel();
      tr::Cx left_cx = left->exp_->UnCx(errormsg);
      tr::Cx right_cx = right->exp_->UnCx(errormsg);

      switch (oper_) {
      case absyn::AND_OP: {
        left_cx.trues_.DoPatch(second_condition_label);
        tr::PatchList true_list = tr::PatchList(right_cx.trues_);
        tr::PatchList false_list =
            tr::PatchList::JoinPatch(left_cx.falses_, right_cx.falses_);
        tree::SeqStm *stm = new tree::SeqStm(
            left_cx.stm_,
            new tree::SeqStm(new tree::LabelStm(second_condition_label),
                             right_cx.stm_));
        return new tr::ExpAndTy(new tr::CxExp(true_list, false_list, stm),
                                type::IntTy::Instance());
        break;
      }

      case absyn::OR_OP: {
        left_cx.falses_.DoPatch(second_condition_label);
        tr::PatchList true_list =
            tr::PatchList::JoinPatch(left_cx.trues_, right_cx.trues_);
        tr::PatchList false_list = tr::PatchList(right_cx.falses_);
        tree::SeqStm *stm = new tree::SeqStm(
            left_cx.stm_,
            new tree::SeqStm(new tree::LabelStm(second_condition_label),
                             right_cx.stm_));
        return new tr::ExpAndTy(new tr::CxExp(true_list, false_list, stm),
                                type::IntTy::Instance());
        break;
      }
      }

    } else {

      tree::CjumpStm *cjump_stm = nullptr;
      tree::RelOp rel_op;
      switch (oper_) {
      case absyn::EQ_OP:
      case absyn::NEQ_OP: {
        rel_op =
            (oper_ == absyn::EQ_OP) ? tree::RelOp::EQ_OP : tree::RelOp::NE_OP;
        // for string comparison, call string_equal
        // String comparison
        // For string equal just calls runtime ???system function stringEqual
        // For string unequal just calls runtime ???system function stringEqual
        // then complements the result
        if (left_ty->IsSameType(type::StringTy::Instance())) {
          tree::ExpList *args = new tree::ExpList();
          args->Append(left->exp_->UnEx());
          args->Append(right->exp_->UnEx());
          tree::ConstExp *expected = (oper_ == absyn::EQ_OP)
                                         ? new tree::ConstExp(1)
                                         : new tree::ConstExp(0);
          // string_equal return 1 for equal, 0 for not
          cjump_stm = new tree::CjumpStm(
              tree::RelOp::EQ_OP, frame::ExternalCall("string_equal", args),
              expected, nullptr, nullptr);
        } else {
          cjump_stm = new tree::CjumpStm(rel_op, left->exp_->UnEx(),
                                         right->exp_->UnEx(), nullptr, nullptr);
        }
      }
      case absyn::LT_OP:
        rel_op = tree::RelOp::LT_OP;
        break;
      case absyn::LE_OP:
        rel_op = tree::RelOp::LE_OP;
        break;
      case absyn::GT_OP:
        rel_op = tree::RelOp::GT_OP;
        break;
      case absyn::GE_OP:
        rel_op = tree::RelOp::GE_OP;
        break;
      case absyn::ABSYN_OPER_COUNT:
        rel_op = tree::RelOp::REL_OPER_COUNT;
        break;
      }

      if (!cjump_stm) {
        cjump_stm = new tree::CjumpStm(rel_op, left->exp_->UnEx(),
                                       right->exp_->UnEx(), nullptr, nullptr);
      }
      tr::PatchList true_list = tr::PatchList({&(cjump_stm->true_label_)});
      tr::PatchList false_list = tr::PatchList({&(cjump_stm->false_label_)});
      return new tr::ExpAndTy(new tr::CxExp(true_list, false_list, cjump_stm),
                              type::IntTy::Instance());
    }
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  DBG("Start translate RecordExp\n");
  // check if record type is defined
  type::Ty *type = tenv->Look(typ_);
  if (type && typeid(*(type->ActualTy())) == typeid(type::RecordTy)) {
    type::RecordTy *recordTy = static_cast<type::RecordTy *>(type->ActualTy());

    // Record creation and initialization
    // { f1 = e1 ; f2 = e2; ??? ; fn = en }

    // A record may outlive the procedure activation that creates it
    // cannot be allocated on the stack, must be allocated on the heap

    // returns the pointer into a new temporary r
    temp::Temp *r = temp::TempFactory::NewTemp();
    tree::ExpList *args = new tree::ExpList();
    // creates an n-word area (CONST n*w)
    args->Append(new tree::ConstExp((recordTy->fields_->GetList().size()) *
                                    reg_manager->WordSize()));
#ifdef GC
    // Call an external memory-allocation function
    // runtime.cc: int *alloc_record(int size, struct string *descriptor)
    // type name + _DESCRIPTOR
    args->Append(new tree::NameExp(
        temp::LabelFactory::NamedLabel(typ_->Name() + "_DESCRIPTOR")));
    tree::MoveStm *left_move_stm = new tree::MoveStm(
        new tree::TempExp(r), frame::ExternalCall("alloc_record", args));
#else
    // Call an external memory-allocation function
    // runtime.c: int *alloc_record(int size)
    tree::MoveStm *left_move_stm = new tree::MoveStm(
        new tree::TempExp(r), frame::ExternalCall("alloc_record", args));
#endif

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

      // A series of MOVE trees can initialize offsets 0, 1w, 2w, ???,(n-1)w
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
  DBG("Start translate SeqExp\n");
  /* TODO: Put your lab5 code here */
  // get the last expression
  std::list<absyn::Exp *> exp_list = seq_->GetList();
  absyn::Exp *last_exp = exp_list.back();
  exp_list.pop_back();

  if (exp_list.empty()) {
    DBG("exp_list has only one element");
    // only one exp in seq_->GetList()
    tr::ExpAndTy *last_exp_and_ty =
        last_exp->Translate(venv, tenv, level, label, errormsg);
    return new tr::ExpAndTy(last_exp_and_ty->exp_, last_exp_and_ty->ty_);
  }

  tree::SeqStm *stm = nullptr;
  tree::SeqStm *current_stm = stm;
  bool first = true;
  for (absyn::Exp *exp : exp_list) {
    if (first) {
      first = false;
      tr::ExpAndTy *exp_and_ty =
          exp->Translate(venv, tenv, level, label, errormsg);
      current_stm = stm = new tree::SeqStm(exp_and_ty->exp_->UnNx(), nullptr);
    } else {
      tr::ExpAndTy *exp_and_ty =
          exp->Translate(venv, tenv, level, label, errormsg);
      current_stm->right_ = new tree::SeqStm(exp_and_ty->exp_->UnNx(), nullptr);
      current_stm = static_cast<tree::SeqStm *>(current_stm->right_);
    }
  }

  tr::ExpAndTy *last_exp_and_ty =
      last_exp->Translate(venv, tenv, level, label, errormsg);

  tree::Exp *exp = nullptr;
  current_stm->right_ = tr::getVoidStm();
  exp = new tree::EseqExp(stm, last_exp_and_ty->exp_->UnEx());

  DBG("Finish translate SeqExp\n");

  // The type and return exp of SeqExp is the type and return exp of the last
  // expression
  return new tr::ExpAndTy(new tr::ExExp(exp), last_exp_and_ty->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  DBG("Start translate AssignExp\n");
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
  DBG("Start translate IfExp\n");
  /* TODO: Put your lab5 code here */

  // If e1 then e2 else e3
  // if(condition) jump t; else jump f;
  // t: e2, f: e3
  DBG("Start translate test_exp_and_ty");
  tr::ExpAndTy *test_exp_and_ty =
      test_->Translate(venv, tenv, level, label, errormsg);
  DBG("Start translate then_exp_and_ty");
  tr::ExpAndTy *then_exp_and_ty =
      then_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *thenTy = then_exp_and_ty->ty_;
  if (errormsg->AnyErrors()) {
    return new tr::ExpAndTy(tr::getVoidExp(), thenTy);
  }

  // Make two labels t and f to which the conditional will branch
  temp::Label *t = temp::LabelFactory::NewLabel();
  temp::Label *f = temp::LabelFactory::NewLabel();
  // Both braches should finish by jumping newly created ???joint??? label
  temp::Label *joint = temp::LabelFactory::NewLabel();

  // Allocate a temporary r
  temp::Temp *r = temp::TempFactory::NewTemp();

  // Treat e1 as a Cx (apply unCx to e1)
  tr::Cx test_cx = test_exp_and_ty->exp_->UnCx(errormsg);

  test_cx.trues_.DoPatch(t);
  test_cx.falses_.DoPatch(f);

  if (!elsee_) {
    // no else
    // then must produce no value
    if (!thenTy->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return new tr::ExpAndTy(tr::getVoidExp(), thenTy);
    }

    // If e2 is ???statements??? (Nx), Translate the result as Nx
    // If not meet condition, go to false label
    tree::SeqStm *stm = new tree::SeqStm(
        test_cx.stm_,
        new tree::SeqStm(new tree::LabelStm(t),
                         new tree::SeqStm(then_exp_and_ty->exp_->UnNx(),
                                          new tree::LabelStm(f))));

    return new tr::ExpAndTy(new tr::NxExp(stm), thenTy);
  } else {
    tr::ExpAndTy *else_exp_and_ty =
        elsee_->Translate(venv, tenv, level, label, errormsg);
    type::Ty *elseTy = else_exp_and_ty->ty_;
    // check if then and else have same type
    if (!elseTy->IsSameType(thenTy)) {
      errormsg->Error(pos_, "then exp and else exp type mismatch");
      return new tr::ExpAndTy(tr::getVoidExp(), thenTy);
    }

    // Treat e2 and e3 as Ex (apply unEx to e2 and e3)

    // after label t,  move e2 to r
    tree::SeqStm *true_stm = new tree::SeqStm(
        new tree::LabelStm(t),
        new tree::SeqStm(
            new tree::MoveStm(new tree::TempExp(r),
                              then_exp_and_ty->exp_->UnEx()),
            new tree::JumpStm(new tree::NameExp(joint),
                              new std::vector<temp::Label *>{joint})));
    // after label f, move e3 to r
    tree::SeqStm *false_stm = new tree::SeqStm(
        new tree::LabelStm(f),
        new tree::SeqStm(
            new tree::MoveStm(new tree::TempExp(r),
                              else_exp_and_ty->exp_->UnEx()),
            new tree::JumpStm(new tree::NameExp(joint),
                              new std::vector<temp::Label *>{joint})));
    // return exp is r
    tree::EseqExp *exp = new tree::EseqExp(
        new tree::SeqStm(
            test_cx.stm_,
            new tree::SeqStm(
                true_stm,
                new tree::SeqStm(false_stm, new tree::LabelStm(joint)))),
        new tree::TempExp(r));
    return new tr::ExpAndTy(new tr::ExExp(exp), thenTy);
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  DBG("Start translate WhileExp\n");
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

  test_cx.trues_.DoPatch(bodyLabel);
  test_cx.falses_.DoPatch(doneLabel);

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
  DBG("Start translate ForExp\n");
  /* TODO: Put your lab5 code here */
  // add id (readonly) to venv
  venv->BeginScope();
#ifdef GC
  venv->Enter(var_,
              new env::VarEntry(tr::Access::AllocLocal(level, false, false),
                                type::IntTy::Instance(), true));
#else
  venv->Enter(var_, new env::VarEntry(tr::Access::AllocLocal(level, false),
                                      type::IntTy::Instance(), true));
#endif

  tr::ExpAndTy *lo_exp_and_ty =
      lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi_exp_and_ty =
      hi_->Translate(venv, tenv, level, label, errormsg);
  // check if lo and hi are int type
  if (!(lo_exp_and_ty->ty_)->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if (!(hi_exp_and_ty->ty_)->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  // temp::Label *loop_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();
  temp::Label *inc_label = temp::LabelFactory::NewLabel();

  // check if body produce value
  tr::ExpAndTy *body_exp_and_ty =
      body_->Translate(venv, tenv, level, done_label, errormsg);
  type::Ty *bodyTy = body_exp_and_ty->ty_;
  if (!bodyTy->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(pos_, "for body should produce no value");
  }

  //    i = lo_
  //    limit = hi_
  //    if i > limit goto done
  //   body:
  //    body
  //    if i == limit goto done
  //   Loop:	i := i + 1
  //        body
  //        if i <= limit goto Loop
  //   done:

  //    i = lo_
  //    limit = hi_
  //    if i > limit goto done
  //   body:
  //    body
  //    if i == limit goto done
  //   inc:
  //    i := i + 1
  //    goto body
  //   done:

  temp::Temp *limit = temp::TempFactory::NewTemp();
  env::VarEntry *loop_i_entry = static_cast<env::VarEntry *>(venv->Look(var_));
  temp::Temp *loop_i =
      (static_cast<frame::InRegAccess *>(loop_i_entry->access_->access_))->reg;

  // init i with lo_
  auto loop_i_init_stmt =
      new tree::MoveStm(new tree::TempExp(loop_i), lo_exp_and_ty->exp_->UnEx());
  // init limit with hi_
  auto limit_init_stmt =
      new tree::MoveStm(new tree::TempExp(limit), hi_exp_and_ty->exp_->UnEx());

  // if i > limit goto done
  auto i_gt_limit_cjump_stmt =
      new tree::CjumpStm(tree::RelOp::GT_OP, new tree::TempExp(loop_i),
                         new tree::TempExp(limit), done_label, body_label);

  // if i == limit goto done
  auto i_eq_limit_cjump_stmt =
      new tree::CjumpStm(tree::RelOp::EQ_OP, new tree::TempExp(loop_i),
                         new tree::TempExp(limit), done_label, inc_label);

  // i := i + 1
  auto loop_i_increase_stmt = new tree::MoveStm(
      new tree::TempExp(loop_i),
      new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(loop_i),
                         new tree::ConstExp(1)));

  tree::Stm *stm = new tree::SeqStm(
      loop_i_init_stmt,
      new tree::SeqStm(
          limit_init_stmt,
          new tree::SeqStm(
              i_gt_limit_cjump_stmt,
              new tree::SeqStm(
                  new tree::LabelStm(body_label),
                  new tree::SeqStm(
                      body_exp_and_ty->exp_->UnNx(),
                      new tree::SeqStm(
                          i_eq_limit_cjump_stmt,
                          new tree::SeqStm(
                              new tree::LabelStm(inc_label),
                              new tree::SeqStm(
                                  loop_i_increase_stmt,
                                  new tree::SeqStm(
                                      new tree::JumpStm(
                                          new tree::NameExp(body_label),
                                          new std::vector<temp::Label *>(
                                              {body_label})),
                                      new tree::LabelStm(done_label))))))))));

  //  // if i == limit goto done
  //  auto i_eq_limit_cjump_stmt =
  //      new tree::CjumpStm(tree::RelOp::EQ_OP, new tree::TempExp(loop_i),
  //                         new tree::TempExp(limit), done_label, loop_label);
  //
  //  // if i <= limit goto Loop
  //  auto i_le_limit_cjump_stmt =
  //      new tree::CjumpStm(tree::RelOp::LE_OP, new tree::TempExp(loop_i),
  //                         new tree::TempExp(limit), loop_label, done_label);

  //  tree::Stm *stm = new tree::SeqStm(
  //      loop_i_init_stmt,
  //      new tree::SeqStm(
  //          limit_init_stmt,
  //          new tree::SeqStm(
  //              i_gt_limit_cjump_stmt,
  //              new tree::SeqStm(
  //                  new tree::LabelStm(body_label),
  //                  new tree::SeqStm(
  //                      body_exp_and_ty->exp_->UnNx(),
  //                      new tree::SeqStm(
  //                          i_eq_limit_cjump_stmt,
  //                          new tree::SeqStm(
  //                              new tree::LabelStm(loop_label),
  //                              new tree::SeqStm(
  //                                  loop_i_increase_stmt,
  //                                  new tree::SeqStm(
  //                                      body_exp_and_ty->exp_->UnNx(),
  //                                      new tree::SeqStm(
  //                                          i_le_limit_cjump_stmt,
  //                                          new tree::LabelStm(
  //                                              done_label)))))))))));

  venv->EndScope();
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  DBG("Start translate BreakExp\n");
  /* TODO: Put your lab5 code here */
  // A break statement simply jump to label
  tree::JumpStm *jump_stm = new tree::JumpStm(
      new tree::NameExp(label), new std::vector<temp::Label *>{label});
  return new tr::ExpAndTy(new tr::NxExp(jump_stm), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  DBG("Start translate LetExp\n");
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm *dec_list_stm = nullptr;
  for (Dec *dec : decs_->GetList()) {
    tree::Stm *dec_stm =
        dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
    if (dec_list_stm) {
      dec_list_stm = new tree::SeqStm(dec_list_stm, dec_stm);
    } else {
      dec_list_stm = dec_stm;
    }
  }

  tr::ExpAndTy *body_exp_and_ty = new tr::ExpAndTy(nullptr, nullptr);
  if (!body_) {
    body_exp_and_ty->ty_ = type::VoidTy::Instance();
    body_exp_and_ty->exp_ = tr::getVoidExp();
  } else
    body_exp_and_ty = body_->Translate(venv, tenv, level, label, errormsg);

  tenv->EndScope();
  venv->EndScope();

  tree::Exp *result_exp;
  if (dec_list_stm) {
    result_exp = new tree::EseqExp(dec_list_stm, body_exp_and_ty->exp_->UnEx());
  } else {
    result_exp = body_exp_and_ty->exp_->UnEx();
  }

  return new tr::ExpAndTy(new tr::ExExp(result_exp),
                          body_exp_and_ty->ty_->ActualTy());
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  DBG("Start translate ArrayExp\n");
  /* TODO: Put your lab5 code here */
  // check if array type is defined
  DBG("Start look typ_ %s in tenv", typ_->Name().data());
  type::Ty *type = tenv->Look(typ_);

  DBG("Finish look typ_ in tenv");
  if (type && typeid(*(type->ActualTy())) == typeid(type::ArrayTy)) {

    DBG("Start translate size_exp_and_ty");
    // check if size is int
    tr::ExpAndTy *size_exp_and_ty =
        size_->Translate(venv, tenv, level, label, errormsg);
    ;
    type::Ty *size_ty = size_exp_and_ty->ty_;
    if (typeid(*size_ty) != typeid(type::IntTy)) {
      errormsg->Error(pos_, "size of array should be int");
      return new tr::ExpAndTy(tr::getVoidExp(), type::VoidTy::Instance());
    }

    DBG("Start translate init_exp_and_ty");
    // check if type of init is same as array
    type::Ty *arrayTy = static_cast<type::ArrayTy *>(type->ActualTy())->ty_;
    tr::ExpAndTy *init_exp_and_ty =
        init_->Translate(venv, tenv, level, label, errormsg);
    type::Ty *initTy = init_exp_and_ty->ty_;
    if (!initTy->IsSameType(arrayTy)) {
      errormsg->Error(pos_, "type mismatch");
      return init_exp_and_ty;
    }

    DBG("Start building args");
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
    DBG("Finish building exp");
    return new tr::ExpAndTy(new tr::ExExp(exp), type->ActualTy());
  } else {
    DBG("undefined array");
    errormsg->Error(pos_, "undefined array %s", typ_);
    return new tr::ExpAndTy(tr::getVoidExp(), type::VoidTy::Instance());
  }
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  DBG("Start translate VoidExp\n");
  /* TODO: Put your lab5 code here */
  return tr::getVoidExpAndVoidTy();
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {

  DBG("Start translate FunctionDec\n");
  // For FunctionDec Node there are two passes for its children nodes

  // First pass
  // Only recursive functions themselves are entered into the venv with their
  // prototypes function name, types of formal parameters, type of return value
  DBG("Start first pass");
  for (absyn::FunDec *function : functions_->GetList()) {

    temp::Label *function_label =
        temp::LabelFactory::NamedLabel(function->name_->Name());

    type::Ty *result_ty = function->result_ ? tenv->Look(function->result_)
                                            : type::VoidTy::Instance();
    type::TyList *formals_ty =
        function->params_->MakeFormalTyList(tenv, errormsg);

    // check if two functions have the same name
    // check only in this declaration list not in environment because
    // only two functions with the same name in the same (consecutive) batch of
    // mutually recursive types is illegal
    for (absyn::FunDec *anotherFunction : functions_->GetList()) {
      if (function != anotherFunction &&
          function->name_ == anotherFunction->name_) {
        errormsg->Error(pos_, "two functions have the same name");
        return new tr::ExExp(new tree::ConstExp(0));
      }
    }

    venv->Enter(function->name_, new env::FunEntry(level, function_label,
                                                   formals_ty, result_ty));
  }

  // Second pass
  // Trans the body using the new environment
  // The formal parameters are processed again
  // This time entering params as env::VarEntrys
  DBG("Start second pass");
  for (absyn::FunDec *function : functions_->GetList()) {

    // get FunEntry
    env::FunEntry *entry =
        static_cast<env::FunEntry *>(venv->Look(function->name_));

    /* TODO: Put your lab5 code here */
    DBG("Start build escape list");
    // build escape list of function formal parameters
    std::list<bool> formal_escapes;
    std::list<bool> store_pointers;
    for (Field *param : function->params_->GetList()) {
      formal_escapes.push_back(param->escape_);
#ifdef GC
      store_pointers.push_back(IsPointer(tenv->Look(param->typ_)));
#endif
    }

    DBG("Creating new nesting level");
    // FunctionDec::Translate creates a new ???nesting level??? for each function
    // by calling tr::Level::NewLevel()
    // Semant keeps this level in its FunEntry
    tr::Level *function_level =
        new tr::Level(level, entry->label_, formal_escapes, store_pointers);
    // Suppose function f(x,y) is nesting inside function g (Level for g is
    // levelg) call tr::Level::NewLevel(levelg, f, {false, false})

    venv->BeginScope();

    type::Ty *result_ty = function->result_ ? tenv->Look(function->result_)
                                            : type::VoidTy::Instance();
    type::TyList *formals_ty =
        function->params_->MakeFormalTyList(tenv, errormsg);

    DBG("Start entering params as env::VarEntry");
    std::list<tr::Access *> *access_list = function_level->Formals();
    // entering params as env::VarEntry
    auto formal_ty = formals_ty->GetList().cbegin();
    auto access = access_list->cbegin();
    for (absyn::Field *param : function->params_->GetList()) {

#ifdef GC
      (*access)->access_->SetStorePointer(IsPointer(tenv->Look(param->typ_)));
#endif

      venv->Enter(param->name_, new env::VarEntry(*access, *formal_ty));
      ++formal_ty;
      ++access;
    }

    DBG("Start translating body");
    // pass function level to body
    tr::ExpAndTy *body_exp_and_ty = function->body_->Translate(
        venv, tenv, function_level, entry->label_, errormsg);
    // check if body_ty is same as result_ty
    if (!body_exp_and_ty->ty_->IsSameType(result_ty)) {
      errormsg->Error(pos_, "procedure returns value");
    }

    venv->EndScope();

    DBG("Calls procEntryExit1 to remember a ProcFrag");
    // Calls procEntryExit1 to remember a ProcFrag
    frags->PushBack(new frame::ProcFrag(
        frame::ProcEntryExit1(
            function_level->frame_,
            new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),
                              body_exp_and_ty->exp_->UnEx())),
        function_level->frame_));
  }
  return tr::getVoidExp();
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  DBG("Start translate VarDec\n");
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *init_exp_and_ty =
      init_->Translate(venv, tenv, level, label, errormsg);
  auto init_ty = init_exp_and_ty->ty_;

  if (typ_) {
    // var x : type_id := exp
    type::Ty *type_id = tenv->Look(typ_);

    // check if type exist
    if (!type_id) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return new tr::NxExp(tr::getVoidStm());
    }

    // check that type_id and type of exp are compatible
    if (!init_ty->ActualTy()->IsSameType(type_id)) {
      errormsg->Error(init_->pos_, "type mismatch");
      return new tr::NxExp(tr::getVoidStm());
    }

#ifdef GC
    DBG_FRAME("Var name %s, is pointer %d", var_->Name().data(), IsPointer(init_ty));
    tr::Access *access =
        tr::Access::AllocLocal(level, escape_, IsPointer(init_ty));
#else
    tr::Access *access = tr::Access::AllocLocal(level, escape_);
#endif

    venv->Enter(var_, new env::VarEntry(access, type_id));

#ifdef GC
    access->access_->SetStorePointer(IsPointer(init_ty));
#endif

    return new tr::NxExp(new tree::MoveStm(
        access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer())),
        init_exp_and_ty->exp_->UnEx()));
  } else {
    // var x := exp
    // if the type of exp is NilTy, must have type_id
    if (init_ty->ActualTy()->IsSameType(type::NilTy::Instance()) &&
        typeid(*(init_ty->ActualTy())) != typeid(type::RecordTy)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
      return new tr::NxExp(tr::getVoidStm());
    }
    // creates a ???new location??? tr::Access for each variable at level level
    // by calling tr::Access::AllocLocal(level, escape_);
#ifdef GC
    DBG_FRAME("Var name %s, is pointer %d", var_->Name().data(), IsPointer(init_ty));
    tr::Access *access =
        tr::Access::AllocLocal(level, escape_, IsPointer(init_ty));
#else
    tr::Access *access = tr::Access::AllocLocal(level, escape_);
#endif
    // Semant records this tr::Access in its VarEntry
    venv->Enter(var_, new env::VarEntry(access, init_ty->ActualTy()));
#ifdef GC
    access->access_->SetStorePointer(IsPointer(init_ty));
#endif
    return new tr::NxExp(new tree::MoveStm(
        access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer())),
        init_exp_and_ty->exp_->UnEx()));
  }
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  DBG("Start translate TypeDec\n");
  /* TODO: Put your lab5 code here */
  DBG("Start first pass");
  for (NameAndTy *type : types_->GetList()) {
    // check if two types have the same name
    // check only in this declaration list not in environment because
    // only two types with the same name in the same (consecutive) batch of
    // mutually recursive types is illegal
    for (NameAndTy *anotherType : types_->GetList()) {
      if (type != anotherType && type->name_ == anotherType->name_) {
        errormsg->Error(pos_, "two types have the same name");
        return tr::getVoidExp();
      }
    }

    DBG("Enter tenv type->name_ %s", type->name_->Name().data());
    // Let the body to be NULL at first
    tenv->Enter(type->name_, new type::NameTy(type->name_, nullptr));
  }

  DBG("Start second pass");
  for (NameAndTy *type : types_->GetList()) {
    // find name_ in tenv
    type::NameTy *name_ty =
        static_cast<type::NameTy *>(tenv->Look(type->name_));

    // modify the ty_ field of the type::NameTy class in the tenv for which is
    // NULL now
    name_ty->ty_ = type->ty_->Translate(tenv, errormsg);

    // doesn't get type
    if (!name_ty->ty_) {
      errormsg->Error(pos_, "undefined type %s", type->name_);
      break;
    }

    DBG("name_ty->sym_ %s, type name %s", name_ty->sym_->Name().data(),
        type->name_->Name().data());

    // check if type declarations form illegal cycle from current one
    type::Ty *tmp = tenv->Look(type->name_), *next, *start = tmp;
    while (tmp) {

      // break if not a name type
      if (typeid(*tmp) != typeid(type::NameTy))
        break;

      next = (static_cast<type::NameTy *>(tmp))->ty_;
      if (next == start) {
        errormsg->Error(pos_, "illegal type cycle");
        return tr::getVoidExp();
      }

      tmp = next;
    }
  }

#ifdef GC
  for (NameAndTy *type : types_->GetList()) {
    // find name_ in tenv
    type::NameTy *name_ty =
        static_cast<type::NameTy *>(tenv->Look(type->name_));

    // add frag to .data segmant for new record type
    if (typeid(*name_ty->ty_->ActualTy()) == typeid(type::RecordTy)) {
      type::RecordTy *record_ty = static_cast<type::RecordTy *>(name_ty->ty_);

      // Lable: "typename_DESCRIPTOR"
      std::string descriptor_label = name_ty->sym_->Name() + "_DESCRIPTOR";
      std::string descriptor_type_str;

      // check fields for pointers, if has pointer, set 1 for according pos;
      // else set 0
      for (const type::Field *field : record_ty->fields_->GetList()) {
        if (IsPointer(field->ty_)) {
          descriptor_type_str += "1";
        } else {
          descriptor_type_str += "0";
        }
      }

      // add to frag
      frags->PushBack(new frame::StringFrag(
          temp::LabelFactory::NamedLabel(descriptor_label),
          descriptor_type_str));
    }
  }
#endif

  return tr::getVoidExp();
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  DBG("Start translate NameTy\n");
  /* TODO: Put your lab5 code here */
  type::Ty *type = tenv->Look(name_);
  if (!type) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return type::NilTy::Instance();
  }
  return type;
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  DBG("Start translate RecordTy\n");
  /* TODO: Put your lab5 code here */
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  DBG("Start translate ArrayTy\n");
  /* TODO: Put your lab5 code here */
  type::Ty *type = tenv->Look(array_);
  if (!type) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    return type::NilTy::Instance();
  }
  return new type::ArrayTy(type);
}

} // namespace absyn
