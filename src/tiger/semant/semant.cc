#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return (static_cast<env::VarEntry *>(entry))->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();

}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  // check if var (lvalue) is record
  type::Ty *type = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*type) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::NilTy::Instance();
  }

  // check if record has this field
  type::RecordTy *record = static_cast<type::RecordTy *>(type);
  for (type::Field *field : record->fields_->GetList()) {
    if (field->name_->Name() == sym_->Name()) {
      return field->ty_;
    }
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return type::NilTy::Instance();

}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  // check if var (lvalue) is array
  type::Ty *type = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(typeid(*type) == typeid(type::ArrayTy)){
    return (static_cast<type::ArrayTy *>(type)->ty_->ActualTy());
  } else {
    errormsg->Error(pos_, "not a array type");
    return type::NilTy::Instance();
  }

}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // check if function is defined
  env::EnvEntry *entry = venv->Look(func_);
  if(entry && typeid(*entry) == typeid(env::FunEntry)){
    env::FunEntry *func = static_cast<env::FunEntry *>(entry);

    // check if formals num match args num
    if(func->formals_->GetList().size() != args_->GetList().size()){
      errormsg->Error(pos_, "params num incorrect in function %s", func_->Name().data());
      return type::VoidTy::Instance();
    }

    // check if types of actual and formal parameters match
    // need to use cbegin for const function
    auto formal_type = func->formals_->GetList().cbegin();
    for(Exp *arg : args_->GetList()){
      type::Ty *actual_type = arg->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!actual_type->IsSameType(*formal_type)){
        errormsg->Error(arg->pos_, "param type does not match");
        return type::VoidTy::Instance();
      }
      ++formal_type;
    }

    // return result type of function
    return func->result_;

  } else {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::VoidTy::Instance();
  }
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP ||
      oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP) {
    // check if operands of arithmetic operators are integers
    if (typeid(*left_ty) != typeid(type::IntTy)) {
      errormsg->Error(left_->pos_,"integer required");
    }
    if (typeid(*right_ty) != typeid(type::IntTy)) {
      errormsg->Error(right_->pos_,"integer required");
    }
    return type::IntTy::Instance();
  } else {
    if (!left_ty->IsSameType(right_ty)) {
      // check if operands type is same
      errormsg->Error(pos_, "same type required");
      return type::IntTy::Instance();
    }
    return type::IntTy::Instance();
  }

}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {

  // check if record type is defined
  type::Ty *type = tenv->Look(typ_);
  if(type && type->ActualTy() && typeid(*(type->ActualTy())) == typeid(type::RecordTy)){
    type::RecordTy *recordTy = static_cast<type::RecordTy *>(type->ActualTy());

    // empty record
    if(fields_->GetList().empty()){
      return type::NilTy::Instance();
    }

    // check if num of fields in RecordExp is same as in definition
    if(fields_->GetList().size() != recordTy->fields_->GetList().size()){
      errormsg->Error(pos_, "num of field doesn't match");
      return type->ActualTy();
    }

    // check if each field in RecordExp is same as in definition
    auto record_field = recordTy->fields_->GetList().cbegin();
    for(absyn::EField *actual_field : fields_->GetList()){
      // check field name
      if(actual_field->name_->Name() != (*record_field)->name_->Name()){
        errormsg->Error(pos_, "field %s doesn't exist", actual_field->name_->Name().data());
      }
      // check field type
      if(!actual_field->exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType((*record_field)->ty_)){
        errormsg->Error(pos_, "type of field %s doesn't match", actual_field->name_->Name().data());
      }
      ++record_field;
    }
    return type->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::NilTy::Instance();
  }
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::TyList *tyList = new type::TyList();
  for (absyn::Exp *exp : seq_->GetList()) {
    type::Ty *ty = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
    tyList->Append(ty);
  }
  // The type of SeqExp is the type of the last expression
  return tyList->GetList().back();
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *expTy = exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  // check if var and exp has the same type
  if(varTy->IsSameType(expTy)){

    // check if loop variable (readonly) is assigned
    // TODO
//    if (typeid(*var_) == typeid(absyn::SimpleVar)) {
//      absyn::SimpleVar *var = static_cast<absyn::SimpleVar *>(var_);
//      env::VarEntry *entry = static_cast<env::VarEntry *>(venv->Look(var->sym_));
//      if (entry->readonly_) {
//        errormsg->Error(pos_, "loop variable can't be assigned");
//      }
//    }

  } else {
    errormsg->Error(pos_, "unmatched assign exp");
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *thenTy = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (errormsg->AnyErrors()) {
    return thenTy;
  }

  if(!elsee_){
    // no else
    // then must produce no value
    if (!thenTy->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(then_->pos_, "if-then exp must produce no value");
    }
    return thenTy;
  } else {
    type::Ty *elseTy = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    // check if then and else have same type
    if(!elseTy->IsSameType(thenTy)){
      errormsg->Error(pos_, "then exp and else exp must have same type");
    }
  }
  return thenTy;

}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  // check if body of while have no value
  type::Ty *bodyTy = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  return type::VoidTy::Instance();
  /* TODO: Put your lab4 code here */
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  for (Dec *dec : decs_->GetList())
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);

  type::Ty *result;
  if (!body_)
    result = type::VoidTy::Instance();
  else
    result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);

  tenv->EndScope();
  venv->EndScope();
  return result;

}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->Enter(var_, new env::VarEntry(init_ty));
  // TODO
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  for(NameAndTy* type : types_->GetList()){

  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
