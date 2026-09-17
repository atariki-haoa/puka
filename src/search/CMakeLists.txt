add_library(puka_search STATIC SearchService.cpp)
target_include_directories(puka_search PUBLIC ${PROJECT_SOURCE_DIR}/src)
