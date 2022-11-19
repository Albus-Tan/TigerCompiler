#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

//#define DEBUG_TRANSLATION

#ifdef DEBUG_TRANSLATION
#define DBG(format, ...) fprintf(stderr, \
"[DEBUG](%s, %s(), Line %d): " \
, __FILE__, __PRETTY_FUNCTION__, __LINE__);     \
fprintf(stderr, format"\r\n", ##__VA_ARGS__)
#else
#define DBG(format, ...)  do {} while (0)
#endif


namespace tr {

class Exp;
class ExpAndTy;
class Level;

class PatchList {
public:
  void DoPatch(temp::Label *label) {
    for (auto &patch : patch_list_)
      *patch = label;
  }

  static PatchList JoinPatch(const PatchList &first, const PatchList &second) {
    PatchList ret(first.GetList());
    for (auto &patch : second.patch_list_) {
      ret.patch_list_.push_back(patch);
    }
    return ret;
  }

  explicit PatchList(std::list<temp::Label **> patch_list)
      : patch_list_(patch_list) {}
  PatchList() = default;

  [[nodiscard]] const std::list<temp::Label **> &GetList() const {
    return patch_list_;
  }

private:
  std::list<temp::Label **> patch_list_;
};

// encapsulate frame::Access
// how to access each var (in frame or reg)
class Access {
public:
  Level *level_;  // level of var
  frame::Access *access_;  // access to var

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}

  static Access *AllocLocal(Level *level, bool escape);

  // get the exp to access the variable, consider static links
  tree::Exp *ToExp(Level *currentLevel);
};

// encapsulate frame::Frame
class Level {
public:
  frame::Frame *frame_;
  Level *parent_;

  Level(Level *parent, temp::Label *name,
        std::list<bool> formals);
  Level(frame::Frame *frame, Level *parent) : frame_(frame), parent_(parent) {}

  std::list<Access *> *Formals();

  // Get the static link from current level to the target level
  tree::Exp *StaticLink(Level *targetLevel);
};

class ProgTr {
public:
  // TODO: Put your lab5 code here */
  ProgTr(std::unique_ptr<absyn::AbsynTree> absyn_tree,
         std::unique_ptr<err::ErrorMsg> errormsg)
      : absyn_tree_(std::move(absyn_tree)), errormsg_(std::move(errormsg)),
        tenv_(std::make_unique<env::TEnv>()),
        venv_(std::make_unique<env::VEnv>()){};

  /**
   * Translate IR tree
   */
  void Translate();

  /**
   * Transfer the ownership of errormsg to outer scope
   * @return unique pointer to errormsg
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<err::ErrorMsg> errormsg_;
  std::unique_ptr<Level> main_level_;
  std::unique_ptr<env::TEnv> tenv_;
  std::unique_ptr<env::VEnv> venv_;

  // Fill base symbol for var env and type env
  void FillBaseVEnv();
  void FillBaseTEnv();
};

} // namespace tr

#endif
