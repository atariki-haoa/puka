#pragma once

// Forward declarations for tree-sitter's opaque types, matching the exact
// `typedef struct X X;` idiom used by <tree_sitter/api.h> itself -- so a
// .cpp file that includes both this header and the real api.h sees two
// identical (legal, redundant) typedef redeclarations, not a conflict. Lets
// syntax/*.hpp headers hold TSXxx* members/params without pulling the full
// tree-sitter API into every file that includes them.
extern "C" {
typedef struct TSLanguage TSLanguage;
typedef struct TSQuery TSQuery;
typedef struct TSParser TSParser;
typedef struct TSTree TSTree;
}
