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
  return type::NilTy::Instance();

}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {

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
    errormsg->Error(pos_, "array type required");
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
  // check if function is defined
  env::EnvEntry *entry = venv->Look(func_);
  if(entry && typeid(*entry) == typeid(env::FunEntry)){
    env::FunEntry *func = static_cast<env::FunEntry *>(entry);

    // check if formals num match args num
    if(func->formals_->GetList().size() != args_->GetList().size()){
      if (args_->GetList().size() > func->formals_->GetList().size()) {
        errormsg->Error(pos_ - 1, "too many params in function " + func_->Name());
        return type::VoidTy::Instance();
      } else {
        errormsg->Error(pos_ - 1, "para type mismatch");
        return type::VoidTy::Instance();
      }
    }

    // check if types of actual and formal parameters match
    // need to use cbegin for const function
    auto formal_type = func->formals_->GetList().cbegin();
    for(Exp *arg : args_->GetList()){
      type::Ty *actual_type = arg->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!actual_type->IsSameType(*formal_type)){
        errormsg->Error(arg->pos_, "para type mismatch");
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
  if(type && typeid(*(type->ActualTy())) == typeid(type::RecordTy)){
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

  type::Ty *varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *expTy = exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  // check if var and exp has the same type
  if(varTy->IsSameType(expTy)){

    // check if loop variable (readonly) is assigned
    if (typeid(*var_) == typeid(absyn::SimpleVar)) {
      absyn::SimpleVar *var = static_cast<absyn::SimpleVar *>(var_);
      env::VarEntry *entry = static_cast<env::VarEntry *>(venv->Look(var->sym_));
      if (entry->readonly_) {
        errormsg->Error(pos_, "loop variable can't be assigned");
      }
    }

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
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
    return thenTy;
  } else {
    type::Ty *elseTy = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    // check if then and else have same type
    if(!elseTy->IsSameType(thenTy)){
      errormsg->Error(pos_, "then exp and else exp type mismatch");
    }
  }
  return thenTy;

}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  // check if body of while have no value
  type::Ty *bodyTy = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!bodyTy->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  // add id (readonly) to venv
  venv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(),true));

  // check if lo and hi are int type
  if (!(lo_->SemAnalyze(venv, tenv, labelcount, errormsg))->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if (!(hi_->SemAnalyze(venv, tenv, labelcount, errormsg))->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  // check if body produce value
  type::Ty *bodyTy = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!bodyTy->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(pos_, "for body should produce no value");
  }

  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  // check if break is out of while or for exp
  if (labelcount == 0) {
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
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
  // check if array type is defined
  type::Ty *type = tenv->Look(typ_);
  if (type && typeid(*(type->ActualTy())) == typeid(type::ArrayTy)) {

    // check if size is int
    type::Ty *size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(typeid(*size_ty)!=typeid(type::IntTy)){
      errormsg->Error(pos_, "size of array should be int");
      return type::VoidTy::Instance();
    }

    // check if type of init is same as array
    type::Ty *arrayTy = static_cast<type::ArrayTy *>(type->ActualTy())->ty_;
    type::Ty *initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);

    if(!initTy->IsSameType(arrayTy)){
      errormsg->Error(pos_, "type mismatch");
      return type->ActualTy();
    }
    return type->ActualTy();

  } else {
    errormsg->Error(pos_, "undefined array %s", typ_);
    return type::VoidTy::Instance();
  }
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  // For FunctionDec Node there are two passes for its children nodes

  // First pass
  // Only recursive functions themselves are entered into the venv with their prototypes
  // function name, types of formal parameters, type of return value
  for(absyn::FunDec *function : functions_->GetList()){
    type::Ty *result_ty = function->result_ ? tenv->Look(function->result_) : type::VoidTy::Instance();
    type::TyList *formals_ty = function->params_->MakeFormalTyList(tenv, errormsg);

    // check if two functions have the same name
    // check only in this declaration list not in environment because
    // only two functions with the same name in the same (consecutive) batch of
    // mutually recursive types is illegal
    for (absyn::FunDec *anotherFunction : functions_->GetList()) {
      if (function != anotherFunction && function->name_ == anotherFunction->name_) {
        errormsg->Error(pos_, "two functions have the same name");
        return;
      }
    }

    venv->Enter(function->name_, new env::FunEntry(formals_ty, result_ty));
  }

  // Second pass
  // Trans the body using the new environment
  // The formal parameters are processed again
  // This time entering params as env::VarEntrys
  for(absyn::FunDec *function : functions_->GetList()){
    venv->BeginScope();

    type::Ty *result_ty = function->result_ ? tenv->Look(function->result_) : type::VoidTy::Instance();
    type::TyList *formals_ty = function->params_->MakeFormalTyList(tenv, errormsg);

    // entering params as env::VarEntry
    auto formal_ty = formals_ty->GetList().cbegin();
    for(absyn::Field *param : function->params_->GetList()){
      venv->Enter(param->name_, new env::VarEntry(*formal_ty));
      ++formal_ty;
    }

    type::Ty *body_ty = function->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    // check if body_ty is same as result_ty
    if(!body_ty->IsSameType(result_ty)){
      errormsg->Error(pos_, "procedure returns value");
    }

    venv->EndScope();
  }


}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if(typ_){
    // var x : type_id := exp
    type::Ty *type_id = tenv->Look(typ_);

    // check if type exist
    if(!type_id){
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return;
    }

    // check that type_id and type of exp are compatible
    if(!init_ty->ActualTy()->IsSameType(type_id)){
      errormsg->Error(init_->pos_, "type mismatch");
      return;
    }

    venv->Enter(var_, new env::VarEntry(type_id));

  } else {
    // var x := exp
    // if the type of exp is NilTy, must have type_id
    if(init_ty->ActualTy()->IsSameType(type::NilTy::Instance()) &&
        typeid(*(init_ty->ActualTy())) != typeid(type::RecordTy)){
      errormsg->Error(pos_, "init should not be nil without type specified");
      return;
    }
    venv->Enter(var_, new env::VarEntry(init_ty->ActualTy()));
  }


}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  for(NameAndTy* type : types_->GetList()){
    // check if two types have the same name
    // check only in this declaration list not in environment because
    // only two types with the same name in the same (consecutive) batch of
    // mutually recursive types is illegal
    for (NameAndTy *anotherType : types_->GetList()) {
      if (type != anotherType && type->name_ == anotherType->name_) {
        errormsg->Error(pos_, "two types have the same name");
        return;
      }
    }
    // Let the body to be NULL at first
    tenv->Enter(type->name_, new type::NameTy(type->name_, nullptr));
  }

  for(NameAndTy* type : types_->GetList()){
    // find name_ in tenv
    type::NameTy *name_ty = static_cast<type::NameTy *>(tenv->Look(type->name_));

    // modify the ty_ field of the type::NameTy class in the tenv for which is NULL now
    name_ty->ty_ = type->ty_->SemAnalyze(tenv, errormsg);

    // doesn't get type
    if(!name_ty->ty_){
      errormsg->Error(pos_, "undefined type %s", type->name_);
      break;
    }

    // check if type declarations form illegal cycle from current one
    type::Ty *tmp = tenv->Look(type->name_), *next, *start = tmp;
    while(tmp){

      // break if not a name type
      if(typeid(*tmp) != typeid(type::NameTy)) break;

      next = (static_cast<type::NameTy *>(tmp))->ty_;
      if(next == start){
        errormsg->Error(pos_, "illegal type cycle");
        return;
      }

      tmp = next;
    }


  }

}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
    type::Ty *type = tenv->Look(name_);
    if(!type){
      errormsg->Error(pos_, "undefined type %s", name_->Name().data());
      return type::NilTy::Instance();
    }
    return type;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  type::Ty *type = tenv->Look(array_);
  if(!type){
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    return type::NilTy::Instance();
  }
  return new type::ArrayTy(type);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
