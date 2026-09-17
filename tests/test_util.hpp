#pragma once
#include <iostream>
#include <string>

namespace puka::test {

// Plain check function instead of assert(): asserts are compiled out under
// NDEBUG (Release builds), which would make these tests silently "pass" with
// no real verification. This runs regardless of build type.
inline int g_failures = 0;

inline void Check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    ++g_failures;
  }
}

}  // namespace puka::test
