add_library(puka_editor STATIC
  Buffer.cpp
  Document.cpp
  DocumentManager.cpp
)
target_include_directories(puka_editor PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(puka_editor PUBLIC puka_syntax)
