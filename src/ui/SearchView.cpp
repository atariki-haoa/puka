#include "ui/SearchView.hpp"

#include <algorithm>
#include <system_error>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace puka {
using namespace ftxui;

SearchView::SearchView(std::filesystem::path root, std::function<void(const SearchHit&)> on_open)
    : root_(std::move(root)), on_open_(std::move(on_open)) {
  InputOption opt;
  opt.content = &query_;
  opt.placeholder = "Search in files";
  opt.multiline = false;  // MUST be false: default true means Enter inserts a literal
                           // '\n' into the query instead of just triggering on_enter.
  opt.on_enter = [this] { PerformSearch(); };
  input_ = Input(opt);
  Add(input_);
}

void SearchView::PerformSearch() {
  selected_ = 0;
  region_ = Region::Query;
  if (query_.empty()) {
    result_ = SearchResult{};
    has_searched_ = false;
    return;
  }
  has_searched_ = true;
  result_ = RunSearch(query_, root_);
}

Element SearchView::OnRender() {
  Element query_row = input_->Render();
  if (region_ == Region::Query) query_row = query_row | color(Color::Cyan);

  std::string status;
  if (!has_searched_) {
    status = "Type a query, press Enter to search";
  } else if (result_.hits.empty()) {
    status = "No matches";
  } else {
    status = std::to_string(result_.hits.size()) + " result" +
              (result_.hits.size() == 1 ? "" : "s");
    if (result_.truncated) status += " (truncated)";
  }
  if (!result_.note.empty()) status += " -- " + result_.note;

  Elements rows;
  for (size_t i = 0; i < result_.hits.size(); ++i) {
    const auto& hit = result_.hits[i];
    std::error_code ec;
    auto rel = std::filesystem::relative(hit.file, root_, ec);
    std::string path_str = ec ? hit.file.string() : rel.string();
    std::string label = path_str + ":" + std::to_string(hit.line) + "  " + hit.preview;
    Element row = text(label);
    if (region_ == Region::Results && static_cast<int>(i) == selected_) row = row | inverted;
    rows.push_back(row);
  }

  Element results_box = rows.empty()
                             ? Element(filler() | flex)
                             : vbox(std::move(rows)) | focusPosition(0, selected_) | frame | flex;

  return vbox({query_row, text(status) | dim, separator(), results_box});
}

bool SearchView::OnEvent(Event event) {
  if (region_ == Region::Query) {
    if (event == Event::ArrowDown && !result_.hits.empty()) {
      region_ = Region::Results;
      selected_ = 0;
      return true;
    }
    return input_->OnEvent(event);
  }

  // region_ == Region::Results
  if (event == Event::ArrowUp) {
    if (selected_ == 0) {
      region_ = Region::Query;
      return true;
    }
    --selected_;
    return true;
  }
  if (event == Event::ArrowDown) {
    selected_ = std::min(selected_ + 1, static_cast<int>(result_.hits.size()) - 1);
    return true;
  }
  if (event == Event::Return) {
    if (on_open_ && !result_.hits.empty()) on_open_(result_.hits[static_cast<size_t>(selected_)]);
    return true;
  }
  return false;
}

}  // namespace puka
