# Running flang-mpi-checker on Another System

`flang-mpi-checker` is source-portable, but it depends on LLVM/Flang 18
development libraries. Those libraries must be installed before the checker
can be built. An MPI installation is not required for the included tests.

## Supported Environments

- Linux with packaged LLVM/Flang 18 development files: recommended.
- Windows through WSL2 with Ubuntu: recommended for Windows users.
- macOS with an LLVM 18 installation that includes Flang libraries: possible,
  but package availability varies.
- Native Windows: possible only with a compatible LLVM/Flang 18 development
  build; WSL2 is substantially easier.

LLVM versions other than 18 are intentionally rejected because Flang's C++ API
changes between LLVM releases.

## Required Software

- A C++17 compiler
- CMake 3.20 or newer
- Ninja
- LLVM 18 development files
- MLIR 18 development files
- Flang 18 compiler, headers, and libraries
- Bash, for `run_tests.sh`

## Ubuntu or Debian

Package names vary by distribution release. On systems using the official LLVM
APT repository, install the LLVM 18 packages:

```bash
sudo apt update
sudo apt install cmake ninja-build clang-18 llvm-18-dev mlir-18-tools \
  libmlir-18-dev flang-18 libflang-18-dev
```

Configure and build:

```bash
cd flang-mpi-checker
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir
cmake --build build --parallel
```

Run the tests:

```bash
bash run_tests.sh
```

If the Flang executable has a different name:

```bash
FLANG_COMPILER=/path/to/llvm-18/bin/flang-new bash run_tests.sh
```

## Fedora, Arch Linux, and Other Linux Distributions

Install or build LLVM, MLIR, and Flang 18, then pass their CMake package
directories explicitly:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/path/to/llvm-18/lib/cmake/llvm \
  -DMLIR_DIR=/path/to/llvm-18/lib/cmake/mlir
cmake --build build --parallel
FLANG_COMPILER=/path/to/llvm-18/bin/flang-new bash run_tests.sh
```

Useful commands for locating installed package directories:

```bash
llvm-config-18 --cmakedir
find /path/to/llvm-18 -type d -path '*/lib/cmake/mlir'
```

## Windows with WSL2

Install Ubuntu under WSL2, then use the Ubuntu instructions above:

```powershell
wsl --install -d Ubuntu
```

Open the Ubuntu terminal, place the project inside the WSL filesystem for
better build performance, and build it as a Linux project.

## macOS

You need an LLVM 18 distribution containing Flang's development libraries.
Apple Clang and the default Apple toolchain do not include them.

After installing or building LLVM/Flang 18:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/path/to/llvm-18/lib/cmake/llvm \
  -DMLIR_DIR=/path/to/llvm-18/lib/cmake/mlir
cmake --build build --parallel
FLANG_COMPILER=/path/to/llvm-18/bin/flang-new bash run_tests.sh
```

## Building LLVM/Flang 18 from Source

Use this when suitable binary development packages are unavailable:

```bash
git clone --branch llvmorg-18.1.8 --depth 1 \
  https://github.com/llvm/llvm-project.git

cmake -S llvm-project/llvm -B llvm-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS="clang;flang;mlir" \
  -DLLVM_TARGETS_TO_BUILD=host
cmake --build llvm-build --parallel
```

Then configure the checker:

```bash
cmake -S flang-mpi-checker -B flang-mpi-checker/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR="$PWD/llvm-build/lib/cmake/llvm" \
  -DMLIR_DIR="$PWD/llvm-build/lib/cmake/mlir"
cmake --build flang-mpi-checker/build --parallel
FLANG_COMPILER="$PWD/llvm-build/bin/flang-new" \
  bash flang-mpi-checker/run_tests.sh
```

## Running the Checker

Compile the provided MPI stub module:

```bash
mkdir -p build/mpi_mods
flang-new-18 -c test/mpi_stub.f90 -o /dev/null -J build/mpi_mods
```

Analyze a Fortran file:

```bash
build/tools/flang-mpi-checker/flang-mpi-checker \
  -I build/mpi_mods path/to/source.f90
```

Use `-v` for extracted-call information and `-output-format=json` or
`-output-format=sarif` for machine-readable output.

For a real application, pass every directory containing required `.mod` files
using additional `-I` options. The checker analyzes source code and does not
need to run the MPI program.

## Troubleshooting

### CMake cannot find LLVM or MLIR

Set `LLVM_DIR` and `MLIR_DIR` to directories containing `LLVMConfig.cmake` and
`MLIRConfig.cmake`.

### CMake reports the wrong LLVM version

The checker requires LLVM/Flang 18. Point `LLVM_DIR` and `MLIR_DIR` at the LLVM
18 installation instead of the system default.

### Flang is not found by the test script

Set `FLANG_COMPILER`:

```bash
FLANG_COMPILER=/absolute/path/to/flang-new bash run_tests.sh
```

### Application modules cannot be found

Compile the application's modules first and pass their output directories with
`-I`. The included tests use `test/mpi_stub.f90`, so they do not require an MPI
installation.
