add_library(puka_ui STATIC
  Icons.cpp
  FileTreeView.cpp
  EditorView.cpp
  SearchView.cpp
  SourceControlView.cpp
  ScmTree.cpp
  DiffView.cpp
  GitStatusBadge.cpp
  Sidebar.cpp
  Layout.cpp
  StatusBar.cpp
  ShortcutsPopup.cpp
)
target_include_directories(puka_ui PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(puka_ui PUBLIC ftxui::ftxui puka_editor puka_fs puka_syntax puka_search puka_git)
