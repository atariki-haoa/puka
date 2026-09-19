#pragma once
#include <string>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/box.hpp>

#include "editor/DocumentManager.hpp"

namespace puka {

class EditorView : public ftxui::ComponentBase {
 public:
  explicit EditorView(DocumentManager& documents);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

  // Opens the find bar for the active document. No-op if none is open --
  // Application's Ctrl+F handler calls this unconditionally, same as how
  // Ctrl+S no-ops with nothing to save.
  void ActivateFind();

  // True while the find bar has input focus -- Application checks this to
  // suppress global commands (Esc in particular, which otherwise means
  // "toggle pane focus"), the same way it already does for the diff view
  // and shortcuts popup.
  bool FindActive() const { return find_active_; }

 private:
  // Re-runs the search from `from_byte` and, on a hit, moves the cursor
  // there (which is what actually scrolls it into view, via OnRender's
  // existing focusPosition-on-cursor). Wraps around the whole buffer if
  // nothing matches between `from_byte` and the relevant end.
  void SearchAndJump(Document& doc, size_t from_byte, bool forward);
  void FindFromScratch();  // query changed -- search forward from where Ctrl+F was pressed
  void FindNext();
  void FindPrev();
  ftxui::Element RenderFindBar();

  DocumentManager& documents_;

  // One box per open tab, captured by OnRender's reflect() so OnEvent can
  // hit-test a mouse click against it -- rebuilt every render, same as
  // DocumentManager::documents() itself.
  std::vector<ftxui::Box> tab_boxes_;

  bool find_active_ = false;
  std::string find_query_;
  size_t find_origin_byte_ = 0;  // cursor position when the find bar opened
  bool find_has_match_ = false;
  size_t find_match_row_ = 0;
  size_t find_match_start_col_ = 0;
  size_t find_match_end_col_ = 0;
};

}  // namespace puka
