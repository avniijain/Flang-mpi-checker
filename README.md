# Flang MPI Checker

`flang-mpi-checker` is a static analysis tool for detecting MPI correctness
problems in Fortran source code. It uses LLVM 18's Flang parser and semantic
analysis, then walks the typed parse tree and applies five MPI-specific rules.

The checker focuses on Fortran information that is difficult to recover from
LLVM IR or inspect with C/C++-only MPI tools: array shape, array sections,
`CONTIGUOUS`, `OPTIONAL`, and derived-type attributes.

## Current Rules

| Rule | Checks |
| --- | --- |
| `BufferSizeRule` | Buffer capacity versus `count * MPI datatype size`, plus statically matchable send/receive envelopes |
| `ContiguityRule` | Strided or non-contiguous array sections and assumed-shape dummies without `CONTIGUOUS` |
| `DerivedTypeRule` | Missing `BIND(C)`/`SEQUENCE`, allocatable or pointer components, and unverified `MPI_Type_create_struct` displacements |
| `OptionalArgRule` | Optional critical MPI arguments passed without an apparent `PRESENT` guard and optional status handling |
| `CollectiveOrderRule` | Collectives inside conditionals and statically visible collective-order conflicts |

## Requirements

- LLVM, MLIR, and Flang **18**
- CMake 3.20 or newer
- Ninja or another CMake-supported build tool
- A C++17 compiler
- Bash and an LLVM 18 Flang executable for `run_tests.sh`

An MPI installation is not required for the included tests. They use the
self-contained module in `test/mpi_stub.f90`.

See [RUNNING_ON_ANY_SYSTEM.md](RUNNING_ON_ANY_SYSTEM.md) for Linux, WSL2,
macOS, and source-build instructions.

## Build

Example for an LLVM 18 installation under `/usr/lib/llvm-18`:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir
cmake --build build --parallel
```

CMake rejects LLVM versions other than 18 because Flang's C++ APIs are not
stable across LLVM releases. The build detects host endianness and defines the
corresponding Flang header macro.

## Run

The checker needs access to the `.mod` files used by the analyzed source. To
analyze a test file with the included MPI stub:

```bash
mkdir -p build/mpi_mods
flang-new-18 -c test/mpi_stub.f90 -o /dev/null -J build/mpi_mods

build/tools/flang-mpi-checker/flang-mpi-checker \
  -I build/mpi_mods test/buffer_mismatch/t01_basic_mismatch.f90
```

Multiple input files and module directories may be supplied:

```bash
build/tools/flang-mpi-checker/flang-mpi-checker \
  -I path/to/mpi/modules -I path/to/application/modules \
  source1.f90 source2.f90
```

### Command-Line Options

- `-I <dir>`: add a Fortran module search directory
- `-output-format=<text|json|sarif>`: select summary/report output
- `-o <file>`: write summary/report output to a file; `-` uses standard output
- `-no-error-on-warning`: return success when only warnings are emitted
- `-max-errors=<n>`: stop processing files after the accumulated error count reaches `n`
- `-v`: print extracted MPI call counts and call kinds

Exit codes are `0` for no errors or warnings, `1` for warnings or frontend
failures, and `2` when checker errors are emitted. `-no-error-on-warning`
changes warning-only runs to exit code `0`.

Diagnostics are emitted as they are found. Text mode prints a summary; JSON and
SARIF serialize the collected diagnostic list.

## Tests

Build the checker first, then run:

```bash
bash run_tests.sh
```

If Flang has a non-standard name or location:

```bash
FLANG_COMPILER=/path/to/llvm-18/bin/flang-new bash run_tests.sh
```

The script compiles `test/mpi_stub.f90` and runs all 22 source fixtures. It
reports frontend execution and diagnostic totals by rule. It is currently a
smoke/regression runner, not a strict expected-diagnostic assertion harness.

The current verified run completes all **22/22** fixtures. Representative tests
exercise all five rule engines, while clean fixtures such as T08, T12, and T19
remain diagnostic-free.

## Project Structure

```text
include/                     Public checker headers and metadata types
lib/MPI/                     MPI call extraction and semantic metadata
lib/Analysis/                Five correctness rule implementations
lib/Diagnostics/             Text, JSON, and SARIF diagnostic output
tools/flang-mpi-checker/     Command-line executable
test/                        MPI stub and 22 Fortran fixtures
run_tests.sh                 Regression/smoke runner
DESIGN.md                    Architecture and design decisions
IMPLEMENTATION.md            LLVM 18 and implementation details
RUNNING_ON_ANY_SYSTEM.md     Cross-system build and run guide
```

## Current Limitations

- Analysis is intra-file and mostly intra-procedural; it does not model rank
  values or full control-flow paths.
- MPI argument positions are currently encoded for a supported subset of MPI
  routines and do not fully model keyword argument reordering.
- Static integer extraction handles literals and resolved named constants, not
  arbitrary constant expressions such as `SIZE`, `STORAGE_SIZE`, or `INT`.
- Send/receive matching is conservative and keyed by communicator name and
  static tag; it does not fully model source, destination, or execution paths.
- `MPI_Type_create_struct` displacements are warned about when they cannot be
  validated; exact component byte offsets are not currently computed.
- Optional-argument guard detection is structural and conservative rather than
  a complete `PRESENT`-aware control-flow analysis.
- Some valid MPI calls cause Flang semantic diagnostics against the permissive
  test stub. The checker intentionally continues when a parse tree and resolved
  symbols are still available.
- Diagnostic source locations are not yet populated by the extractor, so
  checker messages currently use `<unknown>` locations.

