# Implementation and LLVM 18 Details

## Build Integration

The project targets LLVM/Flang 18 and C++17. The root `CMakeLists.txt`:

- requires LLVM and MLIR CMake packages
- rejects LLVM versions other than 18
- imports LLVM definitions and CMake helpers
- detects host endianness and defines `FLANG_LITTLE_ENDIAN=1` or
  `FLANG_BIG_ENDIAN=1`
- uses LLVM-compatible `-fno-rtti` flags on non-MSVC compilers
- builds three checker libraries and the CLI executable

Typical configuration:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir
cmake --build build --parallel
```

The executable links:

- `FlangMPICheckerAnalysis`
- `FlangMPICheckerMPI`
- `FlangMPICheckerDiagnostics`
- `FortranSemantics`
- `FortranEvaluate`
- `FortranParser`
- `FortranDecimal`
- `FortranCommon`
- `LLVMFrontendOpenMP`
- `LLVMFrontendOpenACC`
- `LLVMTargetParser`
- `LLVMSupport`
- `LLVMOption`
- `LLVMDemangle`

## Frontend Pipeline

For each input file, `tools/flang-mpi-checker/main.cpp` creates:

```cpp
Fortran::parser::AllSources allSources;
Fortran::parser::AllCookedSources allCooked(allSources);
Fortran::parser::Parsing parsing(allCooked);
```

It then:

1. adds each CLI `-I` directory to parser search directories
2. calls `parsing.Prescan(filename, parseOptions)`
3. calls `parsing.Parse(llvm::outs())`
4. creates `SemanticsContext` with `AllCookedSources`
5. adds module directories with `set_moduleDirectory()`
6. constructs `Semantics` and calls `Perform()`
7. walks the resulting `parser::Program`
8. runs per-call and end-of-file rules

Fatal prescan or parse errors stop analysis of that file. Semantic errors do
not stop the checker when a usable parse tree remains.

## LLVM 18 API Assumptions

The implementation depends on LLVM 18-specific Flang APIs:

- `std::optional` is used instead of removed `llvm::Optional`
- `Parsing` is constructed with `AllCookedSources`
- `parseTree()` returns `std::optional<parser::Program>&`
- `SemanticsContext` receives `AllCookedSources`
- `DeclTypeSpec` is inspected with `AsIntrinsic()` and `AsDerived()`
- intrinsic kind expressions are evaluated with `evaluate::ToInt64()`
- `CallStmt::call.t` stores the procedure designator and actual-argument list
- `SectionSubscript::u` stores either `IntExpr` or `SubscriptTriplet`
- assumed-size arrays are identified from a starred final upper bound
- resolved parser names expose a raw `Name::symbol` pointer after semantics

These dependencies are why the build rejects other LLVM major versions.

## MPI Call Extraction

`MPICallExtractor` walks `parser::CallStmt` nodes and normalizes recognized
callee names with `mpiCallKindFromName()`.

### Argument Mapping

`buildArgMap()` records positional indexes for supported MPI calls. The current
map includes common point-to-point and collective routines such as
`MPI_Send`, `MPI_Recv`, `MPI_Isend`, `MPI_Irecv`, `MPI_Bcast`,
`MPI_Reduce`, `MPI_Allreduce`, `MPI_Gather`, `MPI_Scatter`,
`MPI_Allgather`, and `MPI_Alltoall`.

Calls recognized by `MPICallKind` but absent from this map can still
participate in kind-level rules, but they do not receive complete argument
metadata.

### Symbol Resolution

The primary resolution path walks a parser actual argument and reads the first
resolved `parser::Name::symbol`. From that symbol:

- `GetUltimate()` follows aliases and use association
- `ObjectEntityDetails::init()` provides named constant values
- `ObjectEntityDetails::shape()` provides explicit and assumed shape
- `GetType()->AsIntrinsic()` provides intrinsic element kind
- `GetType()->AsDerived()` provides a `DerivedTypeSpec`
- `DerivedTypeDetails::componentNames()` provides component order
- symbol attributes provide `CONTIGUOUS`, `OPTIONAL`, `ALLOCATABLE`,
  `POINTER`, and `BIND_C`

Integer literals are read from `IntLiteralConstant` source text. Named
parameters are evaluated with `evaluate::ToInt64()`.

### Buffer Descriptors

`descriptorFromSymbol()` constructs `ArrayDescriptor` metadata:

- rank
- element byte size from intrinsic kind
- explicit lower/upper bounds and extents
- assumed-shape state
- `CONTIGUOUS` state
- symbol-level contiguity

A parser walk then finds `ArrayElement` nodes with triplet subscripts and
classifies section contiguity. A triplet with a stride is currently
`MaybeContiguous`; a scalar subscript before a later triplet identifies a
non-contiguous row-like section.

### Derived-Type Layout

`getDerivedTypeLayout()` records:

- type name
- `BIND(C)` and `SEQUENCE`
- component names
- allocatable and pointer component flags

Component byte sizes and offsets remain `-1`; exact physical layout is not
currently computed. Therefore `MPI_Type_create_struct` produces a conservative
warning rather than validating hard-coded displacements exactly.

### Optional Arguments

Every MPI actual argument is checked for a resolved symbol with `OPTIONAL`.
The extractor records its name and a conservative `mayBeAbsent` value based on
structural conditional nesting. This is not a complete analysis of
`PRESENT(arg)` expressions or control-flow dominance.

### Conditional and Loop Tracking

The parse-tree visitor tracks:

- block `IF` regions through `IfThenStmt` and `EndIfStmt`
- single-line `IfStmt`
- `DoConstruct`

Collective calls record whether they were encountered inside a conditional.

## Correctness Rules

`RuleRunner` executes five rules.

### BufferSizeRule

- compares known buffer bytes with `count * datatypeBytes`
- warns when an assumed-shape buffer cannot be sized
- matches static send/receive envelopes by communicator name and tag

The send/receive comparison does not model ranks, sources, destinations, or
path feasibility.

### ContiguityRule

- errors on statically classified non-contiguous sections
- warns on sections with unknown compile-time contiguity
- warns on assumed-shape dummies without `CONTIGUOUS`

### DerivedTypeRule

- requires derived-type buffers to use `BIND(C)` or `SEQUENCE`
- rejects allocatable and pointer components
- warns when `MPI_Type_create_struct` displacements cannot be validated

### OptionalArgRule

- errors for optional critical arguments that may be absent
- warns when an optional status argument is passed to MPI

Critical argument classification currently uses common argument-name
substrings such as `buf`, `count`, and `datatype`.

### CollectiveOrderRule

- warns for collectives inside conditionals
- compares recorded collective sequence positions on the same communicator

This is structural translation-unit analysis, not rank-aware control-flow
proof.

## Diagnostics and CLI

LLVM utilities used by the CLI include:

- `llvm::InitLLVM`
- `llvm::cl::opt` and `llvm::cl::list`
- `llvm::ToolOutputFile`
- `llvm::SourceMgr`
- `llvm::raw_ostream`
- `llvm::formatv`

`DiagnosticEngine` emits diagnostics immediately and stores them for a final
text summary, JSON array, or SARIF 2.1.0 report. JSON and SARIF currently
contain severity/rule/message information but no source locations.

The extractor does not yet populate `llvm::SMLoc`, so checker diagnostics
currently display `<unknown>`.

## Testing

`run_tests.sh`:

1. locates Flang from `FLANG_COMPILER`, `flang-new-18`, `flang-new`, or `flang`
2. compiles `test/mpi_stub.f90` into `build/mpi_mods`
3. runs all 22 fixtures with that module directory
4. reports diagnostic totals and rule names

The script is a smoke/regression runner. It invokes every fixture and reports
diagnostic totals, but it does not currently assert the checker exit code or
the exact expected rule and diagnostic count for every file.

The current verified run completes 22/22 fixtures and representative fixtures
emit diagnostics from all five rules.

## Implementation Limits and Next Steps

- Populate source locations from Flang provenance into `llvm::SMLoc`.
- Replace positional-only argument mapping with keyword-aware mapping.
- Fold arbitrary integer expressions, including `SIZE`, `STORAGE_SIZE`, and
  conversions.
- Track procedure scopes and implement interprocedural wrapper propagation.
- Compute derived-type component sizes and physical offsets.
- Add rank-aware CFG analysis for optional guards and collective ordering.
- Model coarray synchronization interactions.
- Turn `run_tests.sh` or CTest into a strict expected-diagnostic test harness.
