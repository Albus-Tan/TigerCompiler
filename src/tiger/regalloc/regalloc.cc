#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

//#define REG_ALLOC_LOG(fmt, args...)                                            \
//  do {                                                                         \
//  } while (0);

#define DBG_GRAPH

#define REG_ALLOC_LOG(fmt, args...)                                            \
  do {                                                                         \
    printf("[REG_ALLOC_LOG][%s:%d:%s] " fmt "\n", __FILE__, __LINE__,          \
           __FUNCTION__, ##args);                                              \
    fflush(stdout);                                                            \
  } while (0);

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result() {}

RegAllocator::RegAllocator(frame::Frame *frame,
                           std::unique_ptr<cg::AssemInstr> assem_instr)
    : frame_(frame), assem_instr_(assem_instr->GetInstrList()) {
  Init();
  K = reg_manager->Registers()->GetList().size();
}

void RegAllocator::RegAlloc() {
  REG_ALLOC_LOG("start RegAlloc");
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
    result_ = std::make_unique<Result>(
        color_assign_result.coloring,
        RemoveRedundantMove(color_assign_result.coloring));
  }
}

bool RegAllocator::IsRedundant(assem::Instr *instr, temp::Map *coloring) {
  // check if color of src and dst are the same
  if (typeid(*instr) != typeid(assem::MoveInstr))
    return false;
  auto move_instr = static_cast<assem::MoveInstr *>(instr);
  auto src = move_instr->src_;
  auto dst = move_instr->dst_;
  if (!src || !dst)
    return false;
  if (src->GetList().size() != 1 || dst->GetList().size() != 1)
    return false;
  auto src_color = coloring->Look(src->GetList().front());
  auto dst_color = coloring->Look(dst->GetList().front());
  return (src_color == dst_color);
}

assem::InstrList *RegAllocator::RemoveRedundantMove(temp::Map *coloring) {
  assem::InstrList *new_instr_list = new assem::InstrList();
  for (auto instr : assem_instr_->GetList()) {
    if (!IsRedundant(instr, coloring)) {
      new_instr_list->Append(instr);
    }
  }
  return new_instr_list;
}

void RegAllocator::LivenessAnalysis() {
  REG_ALLOC_LOG("start");

  // construct and assem flow graph
  fg::FlowGraphFactory flow_graph_factory(assem_instr_);
  flow_graph_factory.AssemFlowGraph();
  REG_ALLOC_LOG("finish construct and assem flow graph")

  // construct and assem live graph
  live::LiveGraphFactory live_graph_factory(flow_graph_factory.GetFlowGraph());
  live_graph_factory.Liveness();
  live::LiveGraph live_graph = live_graph_factory.GetLiveGraph();
  interf_graph = live_graph.interf_graph;
  moves = live_graph.moves;
  temp_node_map = live_graph_factory.GetTempNodeMap();
  REG_ALLOC_LOG("finish construct and assem live graph")

#ifdef DBG_GRAPH
  interf_graph->Show(stdout, interf_graph->Nodes(),
                     [&](temp::Temp *t) {  fprintf(stdout, "[t%d]", t->Int()); });
#endif

  worklist_moves = moves;
}
void RegAllocator::Build() {
  REG_ALLOC_LOG("start");

  ClearAndInit();

  temp::Map *temp_map = reg_manager->temp_map_;
  for (live::INodePtr node : interf_graph->Nodes()->GetList()) {
    // init degree
    degree->Enter(node, new int(node->OutDegree()));

    live::MoveList *related_moves = new live::MoveList();
    for (auto move : worklist_moves->GetList()) {
      if (move.first == node || move.second == node) {
        // moveList[n] ← moveList[n] ∪ {I}
        related_moves->Append(move.first, move.second);
      }
    }
    move_list->Enter(node, related_moves);

    // init alias
    alias->Enter(node, node);

    // add temp into precolored or initial
    if (temp_map->Look(node->NodeInfo())) {
      // real register
      REG_ALLOC_LOG("precolored append temp %d", node->NodeInfo()->Int())
      precolored->Append(node);
    } else {
      initial->Append(node);
    }
  }
}
void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  // FIXME: add edge in adjSet
  if (!u->Adj(v) && u != v) {
    // FIXME: precolored add in ?
    if (!precolored->Contain(u)) {
      interf_graph->AddEdge(u, v);
      (*(degree->Look(u)))++;
    }
    if (!precolored->Contain(v)) {
      interf_graph->AddEdge(v, u);
      (*(degree->Look(v)))++;
    }
  }
}
void RegAllocator::MakeWorklist() {
  REG_ALLOC_LOG("start");
  for (auto n : initial->GetList()) {
    if (*(degree->Look(n)) >= K) {
      REG_ALLOC_LOG("Add temp %d to spill_worklist, degree %d",
                    n->NodeInfo()->Int(), *(degree->Look(n)));
      spill_worklist->Union(n);
    } else if (MoveRelated(n)) {
      REG_ALLOC_LOG("Add temp %d to freeze_worklist", n->NodeInfo()->Int());
      freeze_worklist->Union(n);
    } else {
      REG_ALLOC_LOG("Add temp %d to simplify_worklist", n->NodeInfo()->Int());
      simplify_worklist->Union(n);
    }
  }
  initial->Clear();
  REG_ALLOC_LOG("finish");
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
  REG_ALLOC_LOG("start")
  if (simplify_worklist->GetList().empty())
    return;
  // let n ∈ simplifyWorkList
  auto n = simplify_worklist->GetList().front();
  // simplifyWorkList ← simplifyWorkList\{n}
  simplify_worklist->DeleteNode(n);
  // push(n, selectStack)
  REG_ALLOC_LOG("push temp %d on selectStack", n->NodeInfo()->Int())
  select_stack->Prepend(n);

  // forall m ∈ Adjacent(n)
  for (auto m : Adjacent(n)->GetList()) {
    // DecrementDegree(m)
    DecrementDegree(m);
  }
}
void RegAllocator::DecrementDegree(live::INodePtr m) {

  REG_ALLOC_LOG("DecrementDegree temp %d, degree %d", m->NodeInfo()->Int(), *(degree->Look(m)))

  if (precolored->Contain(m)) {
    REG_ALLOC_LOG("DecrementDegree temp %d is precolored", m->NodeInfo()->Int())
    return;
  }

  auto d = degree->Look(m);
  // degree->Set(m, new int((*d)-1));
  --(*d);

  REG_ALLOC_LOG("DecrementDegree, after decrease temp %d, degree %d", m->NodeInfo()->Int(), *(degree->Look(m)))

  if (*d == K) {
    // EnableMoves(m ∪ Adjcent(m))
    live::INodeListPtr m_set = new live::INodeList();
    m_set->Append(m);
    EnableMoves(m_set->Union(Adjacent(m)));
    delete m_set;

    // spillWorkList ← spillWorkList \ {m}
    REG_ALLOC_LOG("Delete temp %d from spill_worklist", m->NodeInfo()->Int());
    spill_worklist->DeleteNode(m);

    if (MoveRelated(m)) {
      freeze_worklist->Union(m);
    } else {
      REG_ALLOC_LOG("Add temp %d to simplify_worklist", m->NodeInfo()->Int());
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
  REG_ALLOC_LOG("start");
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
    REG_ALLOC_LOG("AddWorklist temp %d", u->NodeInfo()->Int());
    AddWorkList(u);
  } else if (precolored->Contain(v) || u->Adj(v)) {
    constrained_moves->Union(x, y);
    REG_ALLOC_LOG("AddWorklist temp %d", u->NodeInfo()->Int());
    AddWorkList(u);
    REG_ALLOC_LOG("AddWorklist temp %d", v->NodeInfo()->Int());
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
      then_clause = Conservative(Adjacent(u)->Union(Adjacent(v)));
    }

    if (then_clause) {
      coalesced_moves->Union(x, y);
      Combine(u, v);
      REG_ALLOC_LOG("AddWorklist temp %d", u->NodeInfo()->Int());
      AddWorkList(u);
    } else {
      active_moves->Union(x, y);
    }
  }
}

void RegAllocator::AddWorkList(live::INodePtr u) {
  if (!precolored->Contain(u) && !MoveRelated(u) && (*(degree->Look(u)) < K)) {
    freeze_worklist->DeleteNode(u);
    REG_ALLOC_LOG("Add temp %d to simplify_worklist, degree %d",
                  u->NodeInfo()->Int(), *(degree->Look(u)));
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
  REG_ALLOC_LOG("start combine temp %d and temp %d", u->NodeInfo()->Int(),
                v->NodeInfo()->Int());
  if (freeze_worklist->Contain(v)) {
    freeze_worklist->DeleteNode(v);
  } else {
    REG_ALLOC_LOG("Delete temp %d from spill_worklist", v->NodeInfo()->Int());
    spill_worklist->DeleteNode(v);
  }
  coalesced_nodes->Union(v);
  alias->Set(v, u);
  move_list->Set(u, move_list->Look(u)->Union(move_list->Look(v)));
  auto v_list = new live::INodeList();
  v_list->Append(v);
  EnableMoves(v_list);
  for (auto t : Adjacent(v)->GetList()) {
    AddEdge(t, u);
    DecrementDegree(t);
  }
  if (*(degree->Look(u)) >= K && freeze_worklist->Contain(u)) {
    freeze_worklist->DeleteNode(u);
    REG_ALLOC_LOG("Add temp %d to spill_worklist", u->NodeInfo()->Int());
    spill_worklist->Union(u);
  }
}
void RegAllocator::Freeze() {
  REG_ALLOC_LOG("start");
  if (freeze_worklist->GetList().empty())
    return;
  auto u = freeze_worklist->GetList().front();
  freeze_worklist->DeleteNode(u);
  REG_ALLOC_LOG("Add temp %d to simplify_worklist", u->NodeInfo()->Int());
  simplify_worklist->Union(u);
  FreezeMoves(u);
}
void RegAllocator::FreezeMoves(live::INodePtr u) {
  for (auto m : NodeMoves(u)->GetList()) {
    auto x = m.first;
    auto y = m.second;
    live::INodePtr u, v;
    if (GetAlias(y) == GetAlias(u)) {
      v = GetAlias(x);
    } else {
      v = GetAlias(y);
    }
    active_moves->Delete(x, y);
    frozen_moves->Union(x, y);
    if (NodeMoves(v)->GetList().empty() && *(degree->Look(v)) < K) {
      freeze_worklist->DeleteNode(v);
      REG_ALLOC_LOG("Add temp %d to simplify_worklist", v->NodeInfo()->Int());
      simplify_worklist->Union(v);
    }
  }
}
void RegAllocator::SelectSpill() {
  REG_ALLOC_LOG("start");
  if (spill_worklist->GetList().empty())
    return;
  // TODO: Should use heuristic algorithm
  REG_ALLOC_LOG("Spill_worklist size %zu", spill_worklist->GetList().size());
  live::INodePtr u = spill_worklist->GetList().front();
  REG_ALLOC_LOG("try SelectSpill temp %d", u->NodeInfo()->Int());
  REG_ALLOC_LOG("Delete temp %d from spill_worklist", u->NodeInfo()->Int());
  spill_worklist->DeleteNode(u);
  // TODO
  if (no_spill_temps->Contain(u))
    return;
  REG_ALLOC_LOG("Add temp %d to simplify_worklist", u->NodeInfo()->Int());
  simplify_worklist->Union(u);
  FreezeMoves(u);
  REG_ALLOC_LOG("SelectSpill success temp %d", u->NodeInfo()->Int());
}
col::Result RegAllocator::AssignColor() {
  REG_ALLOC_LOG("start");
  col::Color color;
  REG_ALLOC_LOG("select_stack size %zu", select_stack->GetList().size());
  while (!select_stack->GetList().empty()) {
    // let n = pop(SelectStack)
    auto n = select_stack->GetList().front();
    select_stack->DeleteNode(n);

    // okColors ← [0, … , K-1]
    color.InitOkColors();

    for (auto w : n->Succ()->GetList()) {
      //      if GetAlias[w]∈(ColoredNodes ∪ precolored) then
      //            okColors ← okColors \ {color[GetAlias(w)]}
      auto alias_w = GetAlias(w);
      if ((colored_nodes->Union(precolored))->Contain(alias_w)) {
        color.RemoveOkColor(alias_w);
      }
    }

    if (color.OkColorsEmpty()) {
      REG_ALLOC_LOG("fail to color node, add to spilled_nodes");
      spilled_nodes->Union(n);
    } else {
      REG_ALLOC_LOG("color node, temp %d", n->NodeInfo()->Int());
      colored_nodes->Union(n);
      color.AssignColor(n);
    }
  }
  // assign same color for coalesced_nodes
  for (live::INodePtr n : coalesced_nodes->GetList()) {
    REG_ALLOC_LOG("assign same color for coalesced_nodes, src GetAlias(n) %d, "
                  "dst temp %d",
                  GetAlias(n)->NodeInfo()->Int(), n->NodeInfo()->Int());
    color.AssignSameColor(GetAlias(n), n);
    REG_ALLOC_LOG("AssignSameColor success");
  }
  REG_ALLOC_LOG("finish");
  return color.BuildAndGetResult();
}

void RegAllocator::RewriteProgram() {
  REG_ALLOC_LOG("start");
  live::INodeListPtr new_temps = new live::INodeList();
  no_spill_temps->Clear();
  for (live::INodePtr v : spilled_nodes->GetList()) {
    // Allocate memory locations for each v∈spilledNodes
    frame::InFrameAccess *access = static_cast<frame::InFrameAccess *>(
        frame::Access::AllocLocal(frame_, true));
    // Create a new temporary vi for each definition and each use
    temp::Temp *old_temp = v->NodeInfo();
    temp::Temp *vi = temp::TempFactory::NewTemp();
    // TODO:

    auto new_instr_list = new assem::InstrList();
    // In the program (instructions), insert a store after each
    // definition of a vi , a fetch before each use of a vi
    auto instr_it = assem_instr_->GetList().begin();
    while (instr_it != assem_instr_->GetList().end()) {
      assert(!precolored->Contain(v));

      (*instr_it)->ReplaceTemp(old_temp, vi);

      // insert a fetch before each use of a vi
      if ((*instr_it)->Use()->Contain(vi)) {
        REG_ALLOC_LOG("insert a fetch before each use of a vi");
        std::string ins("movq (" + frame_->name_->Name() + "_framesize" +
                        std::to_string(frame_->offset_) + ")(`s0), `d0");
        new_instr_list->Append(new assem::OperInstr(
            ins, new temp::TempList(vi),
            new temp::TempList(reg_manager->StackPointer()), nullptr));
      }

      new_instr_list->Append(*instr_it);

      // insert a store after each definition of a vi
      if ((*instr_it)->Def()->Contain(vi)) {
        REG_ALLOC_LOG("insert a store after each definition of a vi");
        std::string ins("movq `s0, (" + frame_->name_->Name() + "_framesize" +
                        std::to_string(frame_->offset_) + ")(`d0)");
        new_instr_list->Append(new assem::OperInstr(
            ins, new temp::TempList(reg_manager->StackPointer()),
            new temp::TempList(vi), nullptr));
      }

      ++instr_it;
    }

    assem_instr_ = new_instr_list;

    // Put All the vi into a set newTemps
    live::INodePtr new_node = interf_graph->NewNode(vi);
    new_temps->Union(new_node);
    no_spill_temps->Append(new_node);
  }

  spill_worklist->Clear();
  initial->Clear();
  initial = colored_nodes->Union(coalesced_nodes->Union(new_temps));
  colored_nodes->Clear();
  coalesced_nodes->Clear();
}

void RegAllocator::Init() {
  REG_ALLOC_LOG("start Init");
  precolored = new live::INodeList();
  simplify_worklist = new live::INodeList();
  freeze_worklist = new live::INodeList();
  spill_worklist = new live::INodeList();
  spilled_nodes = new live::INodeList();
  initial = new live::INodeList();
  coalesced_nodes = new live::INodeList();
  select_stack = new live::INodeList();
  colored_nodes = new live::INodeList();
  no_spill_temps = new live::INodeList();

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
  REG_ALLOC_LOG("start ClearAndInit");
  precolored->Clear();
  simplify_worklist->Clear();
  freeze_worklist->Clear();
  spill_worklist->Clear();
  spilled_nodes->Clear();
  initial->Clear();
  coalesced_nodes->Clear();
  select_stack->Clear();
  colored_nodes->Clear();
  no_spill_temps->Clear();

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