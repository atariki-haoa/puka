#pragma once
#include <ftxui/component/component_base.hpp>

#include "editor/DocumentManager.hpp"

namespace puka {

class EditorView : public ftxui::ComponentBase {
 public:
  explicit EditorView(DocumentManager& documents);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

 private:
  DocumentManager& documents_;
};

}  // namespace puka
