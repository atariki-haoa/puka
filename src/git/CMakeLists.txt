add_library(puka_git STATIC GitService.cpp)
target_include_directories(puka_git PUBLIC ${PROJECT_SOURCE_DIR}/src)
# libgit2's real consumable target (`libgit2package`) only exposes headers
# via $<INSTALL_INTERFACE:...>, which is inert under FetchContent (no
# install step runs) -- see the libgit2 FetchContent comment in the
# top-level CMakeLists.txt for why. GitService.hpp is deliberately
# libgit2-type-free (only GitService.cpp includes any <git2/...> header),
# so PRIVATE is correct and sufficient here.
target_include_directories(puka_git PRIVATE ${libgit2_SOURCE_DIR}/include)
target_link_libraries(puka_git PRIVATE libgit2package)
