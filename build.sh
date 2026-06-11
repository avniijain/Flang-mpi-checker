#!/bin/bash
set -e

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir

cmake --build build --parallel

mkdir -p build/mpi_mods

flang-new-18 -c test/mpi_stub.f90 \
  -o /dev/null \
  -J build/mpi_mods

echo "Build completed successfully."
