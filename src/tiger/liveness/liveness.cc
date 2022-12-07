#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

bool SameSet(const std::set<temp::Temp *> &first,
             const std::set<temp::Temp *> &second) {
  if (first.size() != second.size())
    return false;
  auto it_second = second.begin();
  for (auto it_first : first) {
    if (it_first != *it_second)
      return false;
    ++it_second;
  }
  return true;
}

std::set<temp::Temp *> ToSet(const std::list<temp::Temp *> &origin) {
  std::set<temp::Temp *> res;
  for (auto &it : origin)
    res.insert(it);
  return res;
};

temp::TempList *ToTempList(const std::set<temp::Temp *> &origin) {
  temp::TempList *res = new temp::TempList();
  for (auto &it : origin)
    res->Append(it);
  return res;
};

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  for (fg::FNodePtr node : flowgraph_->Nodes()->GetList()) {
    in_->Enter(node, new temp::TempList());
    out_->Enter(node, new temp::TempList());
  }

  bool fixed = false;
  while (!fixed) {
    fixed = true;
    for (fg::FNodePtr node : flowgraph_->Nodes()->GetList()) {
      assem::Instr *instr = node->NodeInfo();
      temp::TempList *use = instr->Use();
      temp::TempList *def = instr->Def();

      // out[s] = U(n ∈ succ[s]) in[n]
      std::set<temp::Temp *> out_set;
      for (fg::FNodePtr succ : node->Succ()->GetList()) {
        out_set.merge(ToSet(in_->Look(succ)->GetList()));
      }

      // in[s] = use[s] U (out[s] – def[s])
      std::set<temp::Temp *> in_set = ToSet(use->GetList());
      std::set<temp::Temp *> def_set = ToSet(def->GetList());
      std::set<temp::Temp *> out_set_ori = ToSet(out_->Look(node)->GetList());
      std::list<temp::Temp *> diff;
      std::set_difference(out_set_ori.begin(), out_set_ori.end(),
                          def_set.begin(), def_set.end(),
                          back_inserter(diff));
      in_set.merge(ToSet(diff));

      std::set<temp::Temp *> in_set_ori = ToSet(in_->Look(node)->GetList());
      // check if in_set and out_set same
      if (!SameSet(in_set, in_set_ori) || !SameSet(out_set, out_set_ori)) {
        fixed = false;
        temp::TempList *in = ToTempList(in_set);
        temp::TempList *out = ToTempList(out_set);
        in_->Set(node, in);
        out_->Set(node, out);
      }
    }
  }
}

void LiveGraphFactory::InterfGraph() {

  // Build precolored InterfGraph
  auto precolored_temps = reg_manager->Registers()->GetList();
  // create nodes for all precolored regs
  for (auto precolored_temp : precolored_temps) {
    INodePtr node = live_graph_.interf_graph->NewNode(precolored_temp);
    temp_node_map_->Enter(precolored_temp, node);
  }
  // add all edges between precolored regs
  for (temp::Temp *temp1 : precolored_temps) {
    for (temp::Temp *temp2 : precolored_temps) {
      if (temp1 != temp2) {
        INodePtr node1 = temp_node_map_->Look(temp1);
        INodePtr node2 = temp_node_map_->Look(temp2);
        live_graph_.interf_graph->AddEdge(node1, node2);
      }
    }
  }

  /* TODO: Put your lab6 code here */
  // create nodes for all temp regs
  for (auto node : flowgraph_->Nodes()->GetList()) {
    assem::Instr *instr = node->NodeInfo();

    // check regs in Use
    for (auto use : instr->Use()->GetList()) {
      if (temp_node_map_->Look(use))
        continue;
      INodePtr new_node = live_graph_.interf_graph->NewNode(use);
      temp_node_map_->Enter(use, new_node);
    }

    // check regs in Def
    for (auto def : instr->Def()->GetList()) {
      if (temp_node_map_->Look(def))
        continue;
      INodePtr new_node = live_graph_.interf_graph->NewNode(def);
      temp_node_map_->Enter(def, new_node);
    }
  }

  // add edges
  for (auto node : flowgraph_->Nodes()->GetList()) {
    assem::Instr *instr = node->NodeInfo();

    // For an instruction that defines a variable a, where the live-out
    // variables are b1, …, bj, the way to add interference edges for it is

    if (typeid(*instr) == typeid(assem::MoveInstr)) {
      // If it is a move instruction a ← c, add (a, b1), …, (a, bj),  for any bj
      // that is not the same as c
      for (auto def : instr->Def()->GetList()) {
        INodePtr def_node = temp_node_map_->Look(def);
        auto out_set = ToSet(out_->Look(node)->GetList());
        auto use_set = ToSet(instr->Use()->GetList());
        std::list<temp::Temp *> b_list;  // out - use
        std::set_difference(out_set.begin(), out_set.end(),
                            use_set.begin(), use_set.end(),
                            back_inserter(b_list));
        for(auto b : b_list){
          INodePtr b_node = temp_node_map_->Look(b);
          live_graph_.interf_graph->AddEdge(def_node, b_node);
          live_graph_.interf_graph->AddEdge(b_node, def_node);
        }

        // for move instr, add in MoveList
        for (temp::Temp *use : instr->Use()->GetList()) {
          INodePtr use_node = temp_node_map_->Look(use);
          live_graph_.moves->Append(use_node, def_node);
        }
      }

    } else {
      // If it is a nonmove instruction, add (a, b1), …, (a, bj)
      // add edges between def and out for non move instr
      for (auto def : instr->Def()->GetList()) {
        INodePtr def_node = temp_node_map_->Look(def);
        for (auto out : out_->Look(node)->GetList()) {
          INodePtr out_node = temp_node_map_->Look(out);
          live_graph_.interf_graph->AddEdge(def_node, out_node);
          live_graph_.interf_graph->AddEdge(out_node, def_node);
        }
      }
    }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
