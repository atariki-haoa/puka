#include "syntax/LanguageRegistry.hpp"

#include <algorithm>
#include <cctype>

#include <tree_sitter/api.h>

#include "syntax/queries/BashHighlights.hpp"
#include "syntax/queries/CHighlights.hpp"
#include "syntax/queries/CppHighlights.hpp"
#include "syntax/queries/JavaScriptHighlights.hpp"
#include "syntax/queries/JavaScriptJsxHighlights.hpp"
#include "syntax/queries/JavaScriptParamsHighlights.hpp"
#include "syntax/queries/JsonHighlights.hpp"
#include "syntax/queries/PythonHighlights.hpp"
#include "syntax/queries/TypeScriptHighlights.hpp"

extern "C" {
const TSLanguage* tree_sitter_c(void);
const TSLanguage* tree_sitter_cpp(void);
const TSLanguage* tree_sitter_python(void);
const TSLanguage* tree_sitter_javascript(void);
const TSLanguage* tree_sitter_typescript(void);
const TSLanguage* tree_sitter_tsx(void);
const TSLanguage* tree_sitter_json(void);
const TSLanguage* tree_sitter_bash(void);
}

namespace puka {

namespace {

std::string Concat(std::initializer_list<std::string_view> parts) {
  std::string out;
  for (auto part : parts) {
    out += part;
    out += '\n';
  }
  return out;
}

std::string LowerExtension(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

}  // namespace

LanguageRegistry::LanguageRegistry() {
  specs_.push_back({"c", {".c"}, tree_sitter_c, Concat({kCHighlightsScm})});
  specs_.push_back({"cpp",
                     {".cpp", ".cc", ".cxx", ".hpp", ".hh", ".hxx", ".h"},
                     tree_sitter_cpp,
                     Concat({kCHighlightsScm, kCppHighlightsScm})});
  specs_.push_back({"python", {".py", ".pyi", ".pyw"}, tree_sitter_python, Concat({kPythonHighlightsScm})});
  specs_.push_back({"javascript",
                     {".js", ".mjs", ".cjs", ".jsx"},
                     tree_sitter_javascript,
                     Concat({kJavaScriptHighlightsScm, kJavaScriptJsxHighlightsScm,
                             kJavaScriptParamsHighlightsScm})});
  // NOTE: kJavaScriptParamsHighlightsScm is deliberately NOT included here.
  // It patterns-matches plain JS's `formal_parameters` destructuring shape,
  // which is structurally incompatible with the typescript/tsx grammars
  // (confirmed via ts_query_new returning TSQueryErrorStructure) --
  // TypeScriptHighlights.hpp already provides its own parameter-highlighting
  // patterns (`required_parameter`/`optional_parameter`) tailored to how
  // these grammars actually structure parameters.
  specs_.push_back({"typescript",
                     {".ts", ".mts", ".cts"},
                     tree_sitter_typescript,
                     Concat({kJavaScriptHighlightsScm, kTypeScriptHighlightsScm})});
  specs_.push_back({"tsx",
                     {".tsx"},
                     tree_sitter_tsx,
                     Concat({kJavaScriptHighlightsScm, kJavaScriptJsxHighlightsScm,
                             kTypeScriptHighlightsScm})});
  specs_.push_back({"json", {".json"}, tree_sitter_json, Concat({kJsonHighlightsScm})});
  specs_.push_back({"bash", {".sh", ".bash"}, tree_sitter_bash, Concat({kBashHighlightsScm})});
}

LanguageRegistry& LanguageRegistry::Instance() {
  static LanguageRegistry instance;
  return instance;
}

const LanguageSpec* LanguageRegistry::Detect(const std::filesystem::path& path) const {
  std::string ext = LowerExtension(path);
  if (ext.empty()) return nullptr;
  for (const auto& spec : specs_) {
    if (std::find(spec.extensions.begin(), spec.extensions.end(), ext) != spec.extensions.end()) {
      return &spec;
    }
  }
  return nullptr;
}

TSQuery* LanguageRegistry::CompiledQuery(const LanguageSpec& spec) {
  auto it = compiled_queries_.find(&spec);
  if (it != compiled_queries_.end()) return it->second;

  uint32_t error_offset = 0;
  TSQueryError error_type = TSQueryErrorNone;
  TSQuery* query = ts_query_new(spec.language_fn(), spec.highlights_query.data(),
                                 static_cast<uint32_t>(spec.highlights_query.size()), &error_offset,
                                 &error_type);
  (void)error_offset;
  (void)error_type;
  // query is nullptr on failure -- cached as such, never crashes callers.
  compiled_queries_[&spec] = query;
  return query;
}

}  // namespace puka
