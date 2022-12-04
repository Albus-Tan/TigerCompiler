#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include <set>

namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {
  /* TODO: Put your lab6 code here */
private:
  std::unique_ptr<col::Result> result_;
  temp::Map *coloring;
  std::set<std::string *> okColors;
public:
  Color();
  void RemoveOkColor(temp::Temp *t);

  std::unique_ptr<col::Result> BuildAndTransferResult();
};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
