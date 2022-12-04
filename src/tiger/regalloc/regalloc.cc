#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result() {}

void RegAllocator::RegAlloc() {
  LivenessAnalysis();
  Build();
  MakeWorklist();
  do {
    if (!simplify_worklist->GetList().empty())
      Simplify();
    else if (!worklist_moves->GetList().empty())
      Coalesce();
    else if (!freeze_worklist->GetList().empty())
      Freeze();
    else if (!spill_worklist->GetList().empty())
      SelectSpill();
  } while (!(simplify_worklist->GetList().empty() &&
             worklist_moves->GetList().empty() &&
             freeze_worklist->GetList().empty() &&
             spill_worklist->GetList().empty()));
  auto color_assign_result = AssignColor();
  if (!spilled_nodes->GetList().empty()) {
    RewriteProgram();
    RegAlloc();
  } else {
    // TODO: fill result_
  }
}
void RegAllocator::LivenessAnalysis() {

  // construct and assem flow graph
  fg::FlowGraphFactory flow_graph_factory(assem_instr_);
  flow_graph_factory.AssemFlowGraph();

  // construct and assem live graph
  live::LiveGraphFactory live_graph_factory(flow_graph_factory.GetFlowGraph());
  live_graph_factory.Liveness();
  live::LiveGraph live_graph = live_graph_factory.GetLiveGraph();
  interf_graph = live_graph.interf_graph;
  moves = live_graph.moves;
  temp_node_map = live_graph_factory.GetTempNodeMap();

  worklist_moves = moves;
}
void RegAllocator::Build() {
  ClearAndInit();

  temp::Map *temp_map = reg_manager->temp_map_;
  for (live::INodePtr node : interf_graph->Nodes()->GetList()) {
    // init degree
    degree->Enter(node, new int(node->OutDegree()));

    live::MoveList *related_moves = new live::MoveList();
    for(auto move: worklist_moves->GetList()){
      if(move.first == node || move.second == node){
        // moveList[n] ← moveList[n] ∪ {I}
        related_moves->Append(move.first, move.second);
      }
    }
    move_list->Enter(node, related_moves);

    // init alias
    alias->Enter(node, node);

    // add temp into precolored or initial
    if(temp_map->Look(node->NodeInfo())){
      // real register
      precolored->Union(node);
    } else {
      initial->Union(node);
    }
  }
}
void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  // FIXME: add edge in adjSet
  if (!u->Adj(v) && u != v) {
    // FIXME: precolored add in ?
    if(!precolored->Contain(u)){
      interf_graph->AddEdge(u, v);
      (*(degree->Look(u)))++;
    }
    if(!precolored->Contain(v)){
      interf_graph->AddEdge(v, u);
      (*(degree->Look(v)))++;
    }
  }
}
void RegAllocator::MakeWorklist() {
  for (auto n : initial->GetList()) {
    initial->DeleteNode(n);
    if (*(degree->Look(n)) >= K) {
      spill_worklist->Union(n);
    } else if (MoveRelated(n)) {
      freeze_worklist->Union(n);
    } else {
      simplify_worklist->Union(n);
    }
  }
  // or initial->Clear() here
}
live::INodeListPtr RegAllocator::Adjacent(live::INodePtr n) {
  // adjList[n] \ (selectStack ∪ coalescedNodes)
  auto adj_list = n->Succ();
  return adj_list->Diff(select_stack->Union(coalesced_nodes));
}
live::MoveList *RegAllocator::NodeMoves(live::INodePtr n) {
  // moveList[n] ∩ (activeMoves ∪ worklistMoves)
  return move_list->Look(n)->Intersect(active_moves->Union(worklist_moves));
}
bool RegAllocator::MoveRelated(live::INodePtr n) {
  // NodeMoves(n) ≠ {}
  return !NodeMoves(n)->GetList().empty();
}
void RegAllocator::Simplify() {
  if (simplify_worklist->GetList().empty())
    return;
  // let n ∈ simplifyWorkList
  auto n = simplify_worklist->GetList().front();
  // simplifyWorkList ← simplifyWorkList\{n}
  simplify_worklist->DeleteNode(n);
  // push(n, selectStack)
  select_stack->Prepend(n);
  // forall m ∈ Adjacent(n)
  for (auto m : Adjacent(n)->GetList()) {
    // DecrementDegree(m)
    DecrementDegree(m);
  }
}
void RegAllocator::DecrementDegree(live::INodePtr m) {

  if (precolored->Contain(m)) {
    return;
  }

  auto d = degree->Look(m);
  // degree->Set(m, new int((*d)-1));
  --(*d);
  if (*d == K) {
    // EnableMoves(m ∪ Adjcent(m))
    live::INodeListPtr m_set = new live::INodeList();
    m_set->Append(m);
    EnableMoves(m_set->Union(Adjacent(m)));
    delete m_set;

    // spillWorkList ← spillWorkList \ {m}
    spill_worklist->DeleteNode(m);

    if (MoveRelated(m)) {
      freeze_worklist->Union(m);
    } else {
      simplify_worklist->Union(m);
    }
  }
}
void RegAllocator::EnableMoves(live::INodeListPtr nodes) {
  for (auto n : nodes->GetList()) {
    for (auto m : NodeMoves(n)->GetList()) {
      if (active_moves->Contain(m.first, m.second)) {
        active_moves->Delete(m.first, m.second);
        worklist_moves->Union(m.first, m.second);
      }
    }
  }
}
void RegAllocator::Coalesce() {
  if (worklist_moves->GetList().empty()) {
    return;
  }
  auto m = worklist_moves->GetList().front();
  auto x = GetAlias(m.first);
  auto y = GetAlias(m.second);
  live::INodePtr u, v;
  if (precolored->Contain(y)) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  worklist_moves->Delete(m.first, m.second);
  if (u == v) {
    coalesced_moves->Union(x, y);
    AddWorkList(u);
  } else if (precolored->Contain(v) || u->Adj(v)) {
    constrained_moves->Union(x, y);
    AddWorkList(u);
    AddWorkList(v);
  } else {
    bool then_clause = false;
    if (precolored->Contain(u)) {
      auto adj_v = Adjacent(v);
      then_clause = true;
      if (adj_v) {
        for (auto t : adj_v->GetList()) {
          then_clause = then_clause && OK(t, u);
        }
      }
    } else {
      then_clause = Conservative(Adjacent(v)->Union(Adjacent(u)));
    }

    if (then_clause) {
      coalesced_moves->Union(x, y);
      Combine(u, v);
      AddWorkList(u);
    } else {
      active_moves->Union(x, y);
    }
  }
}

void RegAllocator::AddWorkList(live::INodePtr u) {
  if (!precolored->Contain(u) && !MoveRelated(u) && *(degree->Look(u)) < K) {
    freeze_worklist->DeleteNode(u);
    simplify_worklist->Union(u);
  }
}
bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return *(degree->Look(t)) < K || precolored->Contain(t) || t->Adj(r);
}

bool RegAllocator::Conservative(live::INodeListPtr nodes) {
  int k = 0;
  for (auto n : nodes->GetList()) {
    if (*(degree->Look(n)) >= K) {
      k = k + 1;
    }
  }
  return (k < K);
}
live::INodePtr RegAllocator::GetAlias(live::INodePtr n) {
  if (coalesced_nodes->Contain(n))
    return GetAlias(alias->Look(n));
  else
    return n;
}
void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  if (freeze_worklist->Contain(v)) {
    freeze_worklist->DeleteNode(v);
  } else {
    spill_worklist->DeleteNode(v);
  }
  coalesced_nodes->Union(v);
  alias->Set(v, u);
  move_list->Set(u, move_list->Look(u)->Union(move_list->Look(v)));
  auto v_list = new live::INodeList();
  v_list->Append(v);
  EnableMoves(v_list);
  for(auto t : Adjacent(v)->GetList()){
    AddEdge(t, u);
    DecrementDegree(t);
  }
  if(*(degree->Look(u)) >= K && freeze_worklist->Contain(u)){
    freeze_worklist->DeleteNode(u);
    spill_worklist->Union(u);
  }
}
void RegAllocator::Freeze() {
  if(freeze_worklist->GetList().empty()) return;
  auto u = freeze_worklist->GetList().front();
  freeze_worklist->DeleteNode(u);
  simplify_worklist->Union(u);
  FreezeMoves(u);
}
void RegAllocator::FreezeMoves(live::INodePtr u) {
  for(auto m : NodeMoves(u)->GetList()){
    auto x = m.first;
    auto y = m.second;
    live::INodePtr u, v;
    if(GetAlias(y) == GetAlias(u)){
      v = GetAlias(x);
    } else {
      v = GetAlias(y);
    }
    active_moves->Delete(x, y);
    frozen_moves->Union(x, y);
    if(NodeMoves(v)->GetList().empty() && *(degree->Look(v)) < K){
      freeze_worklist->DeleteNode(v);
      simplify_worklist->Union(v);
    }

  }
}
void RegAllocator::SelectSpill() {
  if(spill_worklist->GetList().empty()) return;
  // TODO: Should use heuristic algorithm
  live::INodePtr u;
  for(auto it : spill_worklist->GetList()){
    u = it;
    // TODO: do not select temp can be spilled
    if(!precolored->Contain(it)) break;
  }
  spill_worklist->DeleteNode(u);
  simplify_worklist->Union(u);
  FreezeMoves(u);
}
col::Result RegAllocator::AssignColor() {
  while(!select_stack->GetList().empty()){
    // let n = pop(SelectStack)
    auto n = select_stack->GetList().front();
    select_stack->DeleteNode(n);

    // todo: okColors ← [0, … , K-1]

    for(auto w : n->Adj()->GetList()){
//      if GetAlias[w]∈(ColoreNodes∪precolored) then
//            okColors ← okColors \ {color[GetAlias(w)]}

    }
  }
  return col::Result();
}
void RegAllocator::RewriteProgram() {}

void RegAllocator::Init() {
  precolored = new live::INodeList();
  simplify_worklist = new live::INodeList();
  freeze_worklist = new live::INodeList();
  spill_worklist = new live::INodeList();
  spilled_nodes = new live::INodeList();
  initial = new live::INodeList();
  coalesced_nodes = new live::INodeList();
  select_stack = new live::INodeList();

  worklist_moves = new live::MoveList();
  active_moves = new live::MoveList();
  coalesced_moves = new live::MoveList();
  constrained_moves = new live::MoveList();
  frozen_moves = new live::MoveList();

  move_list = new tab::Table<live::INode, live::MoveList>();
  degree = new tab::Table<live::INode, int>();
  alias = new tab::Table<live::INode, live::INode>();
}
void RegAllocator::ClearAndInit() {
  precolored->Clear();
  simplify_worklist->Clear();
  freeze_worklist->Clear();
  spill_worklist->Clear();
  spilled_nodes->Clear();
  initial->Clear();
  coalesced_nodes->Clear();
  select_stack->Clear();

//  delete worklist_moves;
//  worklist_moves = new live::MoveList();
  delete active_moves;
  active_moves = new live::MoveList();
  delete coalesced_moves;
  coalesced_moves = new live::MoveList();
  delete constrained_moves;
  constrained_moves = new live::MoveList();
  delete frozen_moves;
  frozen_moves = new live::MoveList();

  delete move_list;
  delete degree;
  delete alias;
  move_list = new tab::Table<live::INode, live::MoveList>();
  degree = new tab::Table<live::INode, int>();
  alias = new tab::Table<live::INode, live::INode>();
}

} // namespace ra