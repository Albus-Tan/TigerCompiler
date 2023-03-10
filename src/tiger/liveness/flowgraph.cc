#include "tiger/liveness/flowgraph.h"
#include "tiger/codegen/assem.h"

namespace fg {

#define FLOWGRAPH_LOG(fmt, args...)                                            \
  do {                                                                         \
  } while (0);

//
//#define FLOWGRAPH_LOG(fmt, args...)                                            \
//  do {                                                                         \
//    printf("[FLOWGRAPH_LOG][%s:%d:%s] " fmt "\n", __FILE__, __LINE__,          \
//           __FUNCTION__, ##args);                                              \
//    fflush(stdout);                                                            \
//  } while (0);

void FlowGraphFactory::AssemFlowGraph() {

  FLOWGRAPH_LOG("start")

  // AssemFlowGraph() will construct the flow graph and store into flowgraph_
  // Info of each graph::Node is actually a pointer to an assem::Instr
  assem::Instr *prev_instr = nullptr;
  FNodePtr prev_node = nullptr;

  for (assem::Instr *instr : instr_list_->GetList()) {

    FLOWGRAPH_LOG("create node for instr")

    // create node for instr
    FNodePtr node = flowgraph_->NewNode(instr);

//    if (typeid(*instr) == typeid(assem::LabelInstr)) {
//      label_map_->Enter(static_cast<assem::LabelInstr *>(instr)->label_,
//                        node);
//    }

    if (prev_instr) {

      if (typeid(*prev_instr) == typeid(assem::OperInstr)) {
        // add edge if prev_instr is not jump
        if (!(static_cast<assem::OperInstr *>(prev_instr)->jumps_)) {
          flowgraph_->AddEdge(prev_node, node);
        }
      } else { // assem::MoveInstr or assem::LabelInstr
        if (typeid(*prev_instr) == typeid(assem::LabelInstr)) {
          label_map_->Enter(( static_cast<assem::LabelInstr *>(prev_instr))->label_, node);
        }
        flowgraph_->AddEdge(prev_node, node);
      }
    } else {
      // first instr
    }
    prev_instr = instr;
    prev_node = node;
  }

  FLOWGRAPH_LOG("start check jump fields of the instrs")

  // jump fields of the instrs are used to in creating control flow edges
  // add edges for jump
  for (FNodePtr node : flowgraph_->Nodes()->GetList()) {
    assem::Instr *instr = node->NodeInfo();

    // add edge if instr is jump
    if (typeid(*instr) == typeid(assem::OperInstr)) {
      assem::Targets *jumps =
          static_cast<assem::OperInstr *>(instr)->jumps_;
      if (jumps) {
        for (temp::Label *jump_target : *(jumps->labels_)) {
          FNodePtr target = label_map_->Look(jump_target);
          flowgraph_->AddEdge(node, target);
        }
      }
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const { return new temp::TempList(); }

temp::TempList *MoveInstr::Def() const {
  if (dst_)
    return dst_;
  else
    return new temp::TempList();
}

temp::TempList *OperInstr::Def() const {
  if (dst_)
    return dst_;
  else
    return new temp::TempList();
}

temp::TempList *LabelInstr::Use() const { return new temp::TempList(); }



temp::TempList *MoveInstr::Use() const {
  if (src_)
    return src_;
  else
    return new temp::TempList();
}


temp::TempList *OperInstr::Use() const {
  if (src_)
    return src_;
  else
    return new temp::TempList();
}

} // namespace assem
