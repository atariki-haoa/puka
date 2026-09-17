#include "syntax/LanguageRegistry.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

void TestDetectByExtension() {
  const LanguageSpec* cpp = LanguageRegistry::Instance().Detect("foo.cpp");
  Check(cpp != nullptr && cpp->name == "cpp", "foo.cpp detects as cpp");

  const LanguageSpec* py = LanguageRegistry::Instance().Detect("foo.py");
  Check(py != nullptr && py->name == "python", "foo.py detects as python");

  const LanguageSpec* header = LanguageRegistry::Instance().Detect("foo.h");
  Check(header != nullptr && header->name == "cpp", ".h resolves to cpp");

  const LanguageSpec* json = LanguageRegistry::Instance().Detect("foo.json");
  Check(json != nullptr && json->name == "json", "foo.json detects as json");
}

void TestDetectUnrecognizedExtensionIsNullptr() {
  Check(LanguageRegistry::Instance().Detect("foo.unknownext") == nullptr,
        "unrecognized extension returns nullptr, not a crash");
  Check(LanguageRegistry::Instance().Detect("Makefile") == nullptr,
        "no extension at all returns nullptr");
}

void TestCompiledQuerySucceedsForEveryLanguage() {
  for (const char* ext : {".c", ".cpp", ".py", ".js", ".ts", ".tsx", ".json", ".sh"}) {
    const LanguageSpec* spec = LanguageRegistry::Instance().Detect(std::string("x") + ext);
    Check(spec != nullptr, std::string("extension ") + ext + " is recognized");
    if (!spec) continue;
    TSQuery* query = LanguageRegistry::Instance().CompiledQuery(*spec);
    Check(query != nullptr, std::string("query compiles for language: ") + std::string(spec->name));
  }
}

}  // namespace

int main() {
  TestDetectByExtension();
  TestDetectUnrecognizedExtensionIsNullptr();
  TestCompiledQuerySucceedsForEveryLanguage();

  if (g_failures == 0) {
    std::cout << "test_language_registry: all tests passed\n";
    return 0;
  }
  std::cerr << "test_language_registry: " << g_failures << " failure(s)\n";
  return 1;
}
