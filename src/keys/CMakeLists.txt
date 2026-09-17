add_library(puka_keys STATIC
  CommandRegistry.cpp
  KeymapDefaults.cpp
)
target_include_directories(puka_keys PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(puka_keys PUBLIC ftxui::ftxui)
