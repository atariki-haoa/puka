#include "syntax/Highlighter.hpp"

#include <algorithm>
#include <regex>

#include <tree_sitter/api.h>

#include "syntax/LanguageRegistry.hpp"

namespace puka {

namespace {

bool TextMatchesRegex(const std::string& text, const std::string& pattern) {
  try {
    std::regex re(pattern);
    return std::regex_search(text, re);
  } catch (const std::regex_error&) {
    return true;  // unparseable pattern shouldn't hide a capture we don't understand
  }
}

// Predicate types actually present across this language set's vendored
// queries (verified by reading every one of them): #match?/#not-match?
// (by far the most common), and exactly one #eq? (JS's `require` detection,
// paired with #is-not? local). #is?/#is-not? are "general predicates" that
// need full scope/locals analysis to evaluate correctly -- which we don't
// implement -- so any predicate group we can't verify fails CLOSED (treated
// as unsatisfied) rather than open: a rare miss (e.g. `require` not getting
// its special color) is far less wrong than the alternative -- letting an
// unverified, unconditionally-true capture silently override a correct one
// (this is exactly what happened when this path first failed open: the
// require-detection pattern miscolored every plain identifier as a
// function, including named constants, since its real condition was never
// actually checked).
bool PredicatesSatisfied(std::string_view source_text, const TSQuery* query,
                          const TSQueryMatch& match) {
  uint32_t step_count = 0;
  const TSQueryPredicateStep* steps =
      ts_query_predicates_for_pattern(query, match.pattern_index, &step_count);

  auto capture_text = [&](uint32_t capture_id, std::string* out) -> bool {
    for (uint16_t c = 0; c < match.capture_count; ++c) {
      if (match.captures[c].index != capture_id) continue;
      TSNode node = match.captures[c].node;
      uint32_t start = ts_node_start_byte(node);
      uint32_t end = ts_node_end_byte(node);
      if (end < start || end > source_text.size()) return false;
      *out = std::string(source_text.substr(start, end - start));
      return true;
    }
    return false;
  };

  uint32_t group_start = 0;
  for (uint32_t i = 0; i < step_count; ++i) {
    if (steps[i].type != TSQueryPredicateStepTypeDone) continue;
    uint32_t group_end = i;
    if (group_end == group_start || steps[group_start].type != TSQueryPredicateStepTypeString) {
      group_start = group_end + 1;
      continue;
    }

    uint32_t name_len = 0;
    const char* name = ts_query_string_value_for_id(query, steps[group_start].value_id, &name_len);
    std::string_view predicate_name(name, name_len);
    bool is_match = (predicate_name == "match?" || predicate_name == "not-match?");
    bool is_eq = (predicate_name == "eq?" || predicate_name == "not-eq?");

    if (!is_match && !is_eq) return false;  // unverifiable predicate: fail closed

    if (group_end - group_start < 3 || steps[group_start + 1].type != TSQueryPredicateStepTypeCapture) {
      group_start = group_end + 1;
      continue;  // malformed/unexpected shape: ignore rather than crash
    }

    std::string lhs;
    if (!capture_text(steps[group_start + 1].value_id, &lhs)) {
      group_start = group_end + 1;
      continue;  // referenced capture absent from this match: vacuously skip
    }

    if (is_match) {
      if (steps[group_start + 2].type != TSQueryPredicateStepTypeString) {
        group_start = group_end + 1;
        continue;
      }
      uint32_t pattern_len = 0;
      const char* pattern_str =
          ts_query_string_value_for_id(query, steps[group_start + 2].value_id, &pattern_len);
      bool matched = TextMatchesRegex(lhs, std::string(pattern_str, pattern_len));
      if (matched != (predicate_name == "match?")) return false;
    } else {
      std::string rhs;
      bool rhs_ok;
      if (steps[group_start + 2].type == TSQueryPredicateStepTypeCapture) {
        rhs_ok = capture_text(steps[group_start + 2].value_id, &rhs);
      } else {
        uint32_t len = 0;
        const char* s = ts_query_string_value_for_id(query, steps[group_start + 2].value_id, &len);
        rhs = std::string(s, len);
        rhs_ok = true;
      }
      if (!rhs_ok) {
        group_start = group_end + 1;
        continue;
      }
      if ((lhs == rhs) != (predicate_name == "eq?")) return false;
    }
    group_start = group_end + 1;
  }
  return true;
}

}  // namespace

Highlighter::Highlighter(const LanguageSpec& spec) : spec_(&spec) {
  parser_ = ts_parser_new();
  ts_parser_set_language(parser_, spec_->language_fn());
}

Highlighter::~Highlighter() {
  if (tree_) ts_tree_delete(tree_);
  ts_parser_delete(parser_);
}

void Highlighter::ReparseFull(std::string_view full_text) {
  source_text_ = full_text;
  if (tree_) {
    ts_tree_delete(tree_);
    tree_ = nullptr;
  }
  tree_ = ts_parser_parse_string(parser_, nullptr, source_text_.data(),
                                  static_cast<uint32_t>(source_text_.size()));
}

void Highlighter::Edit(const BufferEdit& e, std::string_view new_full_text) {
  if (tree_) {
    TSInputEdit ts_edit{
        static_cast<uint32_t>(e.start_byte),
        static_cast<uint32_t>(e.old_end_byte),
        static_cast<uint32_t>(e.new_end_byte),
        TSPoint{static_cast<uint32_t>(e.start_row), static_cast<uint32_t>(e.start_col)},
        TSPoint{static_cast<uint32_t>(e.old_end_row), static_cast<uint32_t>(e.old_end_col)},
        TSPoint{static_cast<uint32_t>(e.new_end_row), static_cast<uint32_t>(e.new_end_col)},
    };
    ts_tree_edit(tree_, &ts_edit);
  }
  source_text_ = new_full_text;
  TSTree* new_tree = ts_parser_parse_string(parser_, tree_, source_text_.data(),
                                             static_cast<uint32_t>(source_text_.size()));
  if (tree_) ts_tree_delete(tree_);
  tree_ = new_tree;
}

std::vector<HighlightSpan> Highlighter::SpansForByteRange(size_t start_byte, size_t end_byte) const {
  if (!tree_ || end_byte <= start_byte) return {};
  TSQuery* query = LanguageRegistry::Instance().CompiledQuery(*spec_);
  if (!query) return {};

  TSQueryCursor* cursor = ts_query_cursor_new();
  ts_query_cursor_set_byte_range(cursor, static_cast<uint32_t>(start_byte),
                                  static_cast<uint32_t>(end_byte));
  ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree_));

  struct Candidate {
    size_t start, end;
    uint32_t pattern_index;
    std::string_view name;
  };
  std::vector<Candidate> candidates;

  TSQueryMatch match;
  uint32_t capture_index = 0;
  while (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
    if (!PredicatesSatisfied(source_text_, query, match)) continue;
    const TSQueryCapture& cap = match.captures[capture_index];
    uint32_t name_len = 0;
    const char* name = ts_query_capture_name_for_id(query, cap.index, &name_len);
    candidates.push_back({ts_node_start_byte(cap.node), ts_node_end_byte(cap.node),
                           match.pattern_index, std::string_view(name, name_len)});
  }
  ts_query_cursor_delete(cursor);

  // Conflict resolution, verified against tree-sitter's own reference
  // `crates/highlight` implementation: for the exact same span, the LATER
  // pattern (higher pattern_index) wins; for nested (different-size) spans,
  // the narrower one wins for its own range while the broader one continues
  // to apply around it. Both rules collapse into one mechanism: paint
  // broadest-first into a per-byte owner array, so later/narrower painters
  // always overwrite.
  std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
    size_t la = a.end - a.start, lb = b.end - b.start;
    if (la != lb) return la > lb;
    return a.pattern_index < b.pattern_index;
  });

  std::vector<std::string_view> owner(end_byte - start_byte);
  for (const auto& c : candidates) {
    size_t s = std::max(c.start, start_byte);
    size_t e = std::min(c.end, end_byte);
    for (size_t b = s; b < e; ++b) owner[b - start_byte] = c.name;
  }

  std::vector<HighlightSpan> result;
  for (size_t i = 0; i < owner.size();) {
    if (owner[i].empty()) {
      ++i;
      continue;
    }
    size_t j = i + 1;
    while (j < owner.size() && owner[j] == owner[i]) ++j;
    result.push_back({start_byte + i, start_byte + j, owner[i]});
    i = j;
  }
  return result;
}

}  // namespace puka
