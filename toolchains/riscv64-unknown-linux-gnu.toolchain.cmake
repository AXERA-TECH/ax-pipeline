# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
SET (CMAKE_SYSTEM_NAME Linux)
SET (CMAKE_SYSTEM_PROCESSOR riscv64)

# make sure riscv64-unknown-linux-gnu-gcc and riscv64-unknown-linux-gnu-g++ can be found in $PATH:
SET (CMAKE_C_COMPILER   "riscv64-unknown-linux-gnu-gcc")
SET (CMAKE_CXX_COMPILER "riscv64-unknown-linux-gnu-g++")

# set searching rules for cross-compiler
SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
