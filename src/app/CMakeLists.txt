add_library(puka_app STATIC Application.cpp)
target_include_directories(puka_app PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(puka_app PUBLIC ftxui::ftxui puka_ui puka_editor puka_keys puka_fs puka_search puka_git)
