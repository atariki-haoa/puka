#pragma once
#include <filesystem>
#include <functional>
#include <string>

#include <ftxui/component/component_base.hpp>

#include "search/SearchService.hpp"

namespace puka {

class SearchView : public ftxui::ComponentBase {
 public:
  SearchView(std::filesystem::path root, std::function<void(const SearchHit&)> on_open);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

 private:
  enum class Region { Query, Results };

  void PerformSearch();

  std::filesystem::path root_;
  std::function<void(const SearchHit&)> on_open_;

  std::string query_;
  ftxui::Component input_;

  SearchResult result_;
  bool has_searched_ = false;
  int selected_ = 0;
  Region region_ = Region::Query;
};

}  // namespace puka
