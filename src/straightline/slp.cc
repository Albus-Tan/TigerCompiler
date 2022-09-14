#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(stm1->MaxArgs(), stm2->MaxArgs());
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return stm2->Interp(stm1->Interp(t));
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *int_and_table = exp->Interp(t);
  return int_and_table->t->Update(id, int_and_table->i);
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(exps->MaxArgs(), exps->NumExps());
}

Table *A::PrintStm::Interp(Table *t) const {
  return exps->Interp(t)->t;
}

int IdExp::MaxArgs() const { return 0; }

IntAndTable *IdExp::Interp(Table *t) const {
  return new IntAndTable(t->Lookup(id), t);
}

int NumExp::MaxArgs() const { return 0; }

IntAndTable *NumExp::Interp(Table *t) const { return new IntAndTable(num, t); }

int OpExp::MaxArgs() const {
  return std::max(left->MaxArgs(), right->MaxArgs());
}

IntAndTable *OpExp::Interp(Table *t) const {
  IntAndTable *int_and_table = nullptr;
  switch (oper) {
  case PLUS: {
    IntAndTable *int_and_table_left = left->Interp(t);
    IntAndTable *int_and_table_right = right->Interp(int_and_table_left->t);
    int_and_table = new IntAndTable(
        int_and_table_left->i + int_and_table_right->i, int_and_table_right->t);
    break;
  }
  case MINUS: {
    IntAndTable *int_and_table_left = left->Interp(t);
    IntAndTable *int_and_table_right = right->Interp(int_and_table_left->t);
    int_and_table = new IntAndTable(
        int_and_table_left->i - int_and_table_right->i, int_and_table_right->t);
    break;
  }
  case TIMES: {
    IntAndTable *int_and_table_left = left->Interp(t);
    IntAndTable *int_and_table_right = right->Interp(int_and_table_left->t);
    int_and_table = new IntAndTable(
        int_and_table_left->i * int_and_table_right->i, int_and_table_right->t);
    break;
  }
  case DIV: {
    IntAndTable *int_and_table_left = left->Interp(t);
    IntAndTable *int_and_table_right = right->Interp(int_and_table_left->t);
    int_and_table = new IntAndTable(
        int_and_table_left->i / int_and_table_right->i, int_and_table_right->t);
    break;
  }
  default:
    assert(false);
  }
  return int_and_table;
}

int EseqExp::MaxArgs() const {
  return std::max(stm->MaxArgs(), exp->MaxArgs());
}

IntAndTable *EseqExp::Interp(Table *t) const {
  Table* new_table = stm->Interp(t);
  return exp->Interp(new_table);
}

int PairExpList::MaxArgs() const {
  return std::max(exp->MaxArgs(), tail->MaxArgs());
}

int PairExpList::NumExps() const { return 1 + tail->NumExps(); }

IntAndTable *PairExpList::Interp(Table *t) const {
  IntAndTable * int_and_table = exp->Interp(t);
  std::cout << int_and_table->i << ' ';
  return tail->Interp(int_and_table->t);
}

int LastExpList::MaxArgs() const { return exp->MaxArgs(); }

IntAndTable *LastExpList::Interp(Table *t) const {
  IntAndTable * int_and_table = exp->Interp(t);
  std::cout << int_and_table->i << std::endl;
  return int_and_table;
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}

} // namespace A
