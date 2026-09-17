add_library(puka_syntax STATIC
  LanguageRegistry.cpp
  Highlighter.cpp
  Theme.cpp
)
target_include_directories(puka_syntax PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(puka_syntax PUBLIC
  ftxui::ftxui
  tree-sitter
  tree-sitter-c tree-sitter-cpp tree-sitter-python
  tree-sitter-javascript tree-sitter-typescript tree-sitter-tsx
  tree-sitter-json tree-sitter-bash)
