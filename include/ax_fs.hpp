#pragma once

// GCC 7.x (e.g. arm-linux-gnueabihf toolchain) typically ships filesystem under
// <experimental/filesystem> only. Provide a single namespace alias for the codebase.

#if defined(__has_include)
#  if __has_include(<filesystem>)
#    include <filesystem>
namespace axfs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
namespace axfs = std::experimental::filesystem;
#  else
#    error "No filesystem header available"
#  endif
#else
// Fallback: assume C++17 filesystem is available.
#  include <filesystem>
namespace axfs = std::filesystem;
#endif

