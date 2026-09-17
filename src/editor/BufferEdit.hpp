#pragma once
#include <cstddef>

namespace puka {

// Byte-offset + row/col delta of a single edit, in the shape a tree-sitter
// TSInputEdit needs. Row/col pairs are captured directly by Buffer at edit
// time rather than re-derived from byte offsets afterward: old_end_row/col
// describes a position that may no longer exist in the buffer once a
// deletion completes, so it can't be safely recomputed via Buffer::RowCol()
// post-edit. Deliberately tree-sitter-free (plain size_t only) so Buffer
// itself never needs to depend on tree-sitter types.
struct BufferEdit {
  size_t start_byte, old_end_byte, new_end_byte;
  size_t start_row, start_col;
  size_t old_end_row, old_end_col;
  size_t new_end_row, new_end_col;
};

}  // namespace puka
