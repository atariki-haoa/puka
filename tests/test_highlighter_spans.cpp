#include "syntax/Highlighter.hpp"
#include "syntax/LanguageRegistry.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

bool ContainsSpan(const std::vector<HighlightSpan>& spans, size_t start, size_t end,
                   std::string_view capture) {
  for (const auto& s : spans) {
    if (s.start_byte == start && s.end_byte == end && s.capture_name == capture) return true;
  }
  return false;
}

// JSON is the smallest, predicate-free grammar, and its expected result
// locks in the conflict-resolution rule discovered while researching this
// feature: `(pair key: (_) @string.special.key)` appears BEFORE the generic
// `(string) @string` in json's own vendored highlights.scm, and per
// tree-sitter's reference highlight implementation, the LATER pattern wins
// for an identical span -- so the key ends up captured as plain "string",
// not "string.special.key". This is upstream's own behavior, not a bug.
void TestJsonKeyAndValueResolveToPlainString() {
  const LanguageSpec* spec = LanguageRegistry::Instance().Detect("x.json");
  Check(spec != nullptr, "json language is registered");
  if (!spec) return;

  Highlighter hl(*spec);
  std::string src = R"({"a": 1})";
  //                    0123456789
  //                    { " a " :   1 }
  //                    0 1 2 3 4 5 6 7 8
  hl.ReparseFull(src);

  auto spans = hl.SpansForByteRange(0, src.size());
  Check(ContainsSpan(spans, 1, 4, "string"), "key \"a\" resolves to plain string, not string.special.key");
  Check(ContainsSpan(spans, 6, 7, "number"), "value 1 is captured as number");
}

void TestEmptyRangeReturnsNoSpans() {
  const LanguageSpec* spec = LanguageRegistry::Instance().Detect("x.json");
  Highlighter hl(*spec);
  hl.ReparseFull("{}");
  Check(hl.SpansForByteRange(0, 0).empty(), "empty byte range produces no spans");
}

}  // namespace

int main() {
  TestJsonKeyAndValueResolveToPlainString();
  TestEmptyRangeReturnsNoSpans();

  if (g_failures == 0) {
    std::cout << "test_highlighter_spans: all tests passed\n";
    return 0;
  }
  std::cerr << "test_highlighter_spans: " << g_failures << " failure(s)\n";
  return 1;
}
