#include "search/SearchService.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

void TestParseVimgrepLineBasic() {
  auto hit = ParseRipgrepVimgrepLine("src/foo.cpp:42:5:some text");
  Check(hit.has_value(), "basic vimgrep line parses");
  if (!hit) return;
  Check(hit->file == "src/foo.cpp", "vimgrep file field");
  Check(hit->line == 42, "vimgrep line field");
  Check(hit->column == 5, "vimgrep column field");
  Check(hit->preview == "some text", "vimgrep preview field");
}

void TestParseVimgrepLineTextContainsColons() {
  // Real line captured from `rg --vimgrep` against this repo.
  auto hit = ParseRipgrepVimgrepLine(
      "src/editor/Document.hpp:24:3:  std::string DisplayName() const;");
  Check(hit.has_value(), "vimgrep line with colons in the text parses");
  if (!hit) return;
  Check(hit->line == 24, "line field unaffected by colons in text");
  Check(hit->column == 3, "column field unaffected by colons in text");
  Check(hit->preview == "  std::string DisplayName() const;",
        "preview keeps embedded colons intact, only first 3 delimiters consumed");
}

void TestParseVimgrepLineRejectsBinaryFileSummary() {
  // rg --vimgrep prints this (with exit code 0!) for binary files -- must
  // never be mistaken for a real hit.
  auto hit = ParseRipgrepVimgrepLine(
      "bintest.bin: binary file matches (found \"\\0\" byte around offset 5)");
  Check(!hit.has_value(), "binary-file summary line is rejected, not parsed as a hit");
}

void TestParseGrepLineBasic() {
  // Real line captured from `grep -rn` against this repo.
  auto hit = ParseGrepLine(
      "src/ui/Sidebar.hpp:6:enum class SidebarView { Explorer, Search, SourceControl };");
  Check(hit.has_value(), "basic grep line parses");
  if (!hit) return;
  Check(hit->line == 6, "grep line field");
  Check(hit->column == 1, "grep has no column field, defaults to 1");
  Check(hit->preview == "enum class SidebarView { Explorer, Search, SourceControl };",
        "grep preview field");
}

void TestParseGrepLineTextContainsColon() {
  auto hit = ParseGrepLine("a/b.txt:3:key: value: more");
  Check(hit.has_value(), "grep line with colons in text parses");
  if (!hit) return;
  Check(hit->line == 3, "grep line field unaffected by colons in text");
  Check(hit->preview == "key: value: more", "grep preview keeps embedded colons intact");
}

void TestParseLinesRejectMalformedInput() {
  Check(!ParseRipgrepVimgrepLine("too:few:colons").has_value(), "vimgrep rejects too few fields");
  Check(!ParseRipgrepVimgrepLine("a:notanumber:5:text").has_value(),
        "vimgrep rejects non-numeric line field");
  Check(!ParseGrepLine("nocolonatall").has_value(), "grep rejects a line with no colons");
  Check(!ParseGrepLine(":5:text").has_value(), "grep rejects an empty path field");
}

void TestCapResultsTruncatesAtLimit() {
  std::vector<SearchHit> hits;
  for (size_t i = 0; i < 501; ++i) {
    hits.push_back({"f.txt", i + 1, 1, "line"});
  }
  auto result = CapResults(std::move(hits), 500);
  Check(result.hits.size() == 500, "501 hits capped to 500");
  Check(result.truncated, "truncated flag set when over the cap");
  Check(result.hits.front().line == 1, "first hit preserved after truncation");
}

void TestCapResultsNoTruncationUnderLimit() {
  std::vector<SearchHit> hits = {{"f.txt", 1, 1, "a"}, {"f.txt", 2, 1, "b"}, {"f.txt", 3, 1, "c"}};
  auto result = CapResults(hits, 500);
  Check(result.hits.size() == 3, "hits under the cap are unchanged");
  Check(!result.truncated, "truncated flag not set under the cap");
}

}  // namespace

int main() {
  TestParseVimgrepLineBasic();
  TestParseVimgrepLineTextContainsColons();
  TestParseVimgrepLineRejectsBinaryFileSummary();
  TestParseGrepLineBasic();
  TestParseGrepLineTextContainsColon();
  TestParseLinesRejectMalformedInput();
  TestCapResultsTruncatesAtLimit();
  TestCapResultsNoTruncationUnderLimit();

  if (g_failures == 0) {
    std::cout << "test_search_parsing: all tests passed\n";
    return 0;
  }
  std::cerr << "test_search_parsing: " << g_failures << " failure(s)\n";
  return 1;
}
