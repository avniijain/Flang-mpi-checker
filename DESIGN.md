# Design

## Goal

`flang-mpi-checker` detects MPI correctness problems in Fortran source before
the program is executed. It is intentionally built above Flang's source and
semantic layers because many target bugs depend on Fortran concepts that are
lost or obscured after lowering:

- explicit and assumed array shape
- column-major array-section contiguity
- the `CONTIGUOUS` and `OPTIONAL` attributes
- derived-type `BIND(C)` and `SEQUENCE` properties
- allocatable and pointer components
- conditional placement and ordering of collectives

## Analysis Pipeline

```text
Fortran source
    |
    v
Flang prescan and parse
    |
    v
Flang semantic analysis
    |
    v
Typed parse-tree walk
    |
    v
MPICallMetadata / CollectiveCall records
    |
    v
Per-call rules + translation-unit rules
    |
    v
Text diagnostics and text/JSON/SARIF report
```

The executable performs prescan, parsing, and `Semantics::Perform()` for each
input file. If semantic analysis reports errors but leaves a usable parse tree,
the checker continues so that permissive MPI interfaces and partially resolved
programs can still be analyzed.

## Architecture

The project has three library layers and one executable:

- `MPI`: recognizes supported MPI calls and extracts normalized metadata.
- `Analysis`: applies five independent correctness rules.
- `Diagnostics`: collects diagnostics and serializes reports.
- `tools/flang-mpi-checker`: owns CLI parsing and the frontend pipeline.

The central interchange type is `MPICallMetadata`. It records the call kind,
buffer descriptor, static count, MPI datatype size, derived-type layout,
communicator/tag, optional arguments, and control-flow nesting. Collective
calls also produce a smaller `CollectiveCall` record for translation-unit
ordering checks.

## Extraction Strategy

After `Semantics::Perform()`, Flang populates `parser::Name::symbol` pointers.
The current extractor treats those pointers as its primary semantic bridge:

- buffer symbol: walk an actual argument to its first resolved `Name`
- count: parse an integer literal or evaluate a named object's initializer
- MPI datatype: read its source name or resolve its parameter initializer
- array shape: inspect `ObjectEntityDetails::shape()`
- derived type: inspect `DeclTypeSpec::AsDerived()` and `DerivedTypeDetails`
- optional argument: inspect the resolved symbol's `OPTIONAL` attribute
- contiguity: combine symbol attributes with parser-level section subscripts

This design does not require `CallStmt::typedCall` to be available. That is
important because Flang may leave `typedCall` unavailable after semantic
diagnostics from broad or permissive MPI interfaces.

## Rule Model

Rules have two execution phases:

- `check(call)`: local checks on each extracted MPI call
- `checkAll(calls, collectives)`: checks requiring multiple calls in the file

`RuleRunner` registers and executes:

1. `BufferSizeRule`
2. `ContiguityRule`
3. `DerivedTypeRule`
4. `OptionalArgRule`
5. `CollectiveOrderRule`

### BufferSizeRule

Computes a static envelope as `count * datatypeBytes` and compares it with the
buffer's statically known total bytes. It also compares statically matchable
send and receive envelopes sharing a communicator name and tag.

### ContiguityRule

Combines declared symbol properties and parser-level array-section syntax.
Strided sections are potentially non-contiguous, and a scalar subscript before
a later triplet identifies non-contiguous row-like sections in Fortran's
column-major layout. Assumed-shape dummies without `CONTIGUOUS` are warned.

### DerivedTypeRule

For derived-type buffers, checks `BIND(C)` or `SEQUENCE` and rejects allocatable
or pointer components. `MPI_Type_create_struct` emits a warning when explicit
displacements cannot be validated against computed component offsets.

### OptionalArgRule

Finds resolved optional dummy arguments passed into MPI calls. Critical buffer,
count, and datatype-like names are diagnosed when they may be absent. Optional
status arguments receive a separate warning.

### CollectiveOrderRule

Records collective calls and warns when a collective occurs inside a
conditional. It also reports statically visible same-position conflicts on the
same communicator.

## Design Decisions

### Source/Semantic Analysis Instead of LLVM IR

LLVM IR analysis would work on lowered programs but would lose important
Fortran properties or require reconstructing them from descriptors. The
source/semantic approach preserves those properties directly.

### Flang Instead of a Custom Parser

A custom parser would reduce the LLVM dependency but require implementing
modern Fortran parsing, module handling, name resolution, and type semantics.
Flang already supplies those capabilities.

### Metadata Before Rules

Extraction and checking are separate. Rules consume normalized metadata rather
than depending directly on parse-tree node shapes. This keeps rule code small
and allows future extraction improvements without rewriting each rule.

### Conservative Static Results

When a value or layout cannot be proven statically, the checker generally
skips an exact error or emits a warning. It does not claim full path-sensitive,
interprocedural, or runtime MPI analysis.

## Known Design Limits

- The MPI argument map covers a subset of routines and positional calling
  conventions.
- Control-flow tracking is structural, not a full CFG or rank analysis.
- Collective ordering is file-local and conservative.
- Exact derived-type component sizes and offsets are not yet calculated.
- Optional `PRESENT` guard recognition is heuristic.
- Source locations are not yet connected to the diagnostic engine.
- Coarray synchronization and generic-wrapper propagation are not modeled.

These limits are deliberate boundaries of the current implementation and are
the main areas for future analysis work.

