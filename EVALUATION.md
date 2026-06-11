# EVALUATION

## 1. Objective

The goal of this project is to perform static correctness analysis of MPI programs written in Fortran using the Flang compiler infrastructure (LLVM/Flang 18).

Unlike traditional compilers, which primarily detect syntax and type errors, the checker aims to identify MPI-specific correctness issues before program execution.

The implemented rules focus on:

* Buffer size mismatches in MPI communication calls
* Non-contiguous array sections passed to MPI routines
* Unsafe derived types used in MPI communication
* Misuse of optional arguments
* Collective communication ordering violations

---

## 2. Baseline Comparison

To evaluate the usefulness of the checker, its capabilities were compared against standard compilation using Flang.

| Capability                               | Standard Flang Compiler | flang-mpi-checker |
| ---------------------------------------- | ----------------------- | ----------------- |
| Syntax checking                          | Yes                     | Yes               |
| Type checking                            | Yes                     | Yes               |
| MPI buffer mismatch detection            | No                      | Yes               |
| MPI collective ordering analysis         | No                      | Yes               |
| Derived-type communication safety checks | No                      | Yes               |
| Array contiguity checks                  | No                      | Yes               |
| Optional MPI argument validation         | No                      | Yes               |

The baseline compiler successfully compiles MPI programs but does not perform semantic MPI correctness analysis. The proposed checker provides additional diagnostics targeted at common MPI programming errors.

---

## 3. Test Suite

A suite of 22 test programs was developed to evaluate the implemented analysis rules.

### Buffer Mismatch Tests

* t01_basic_mismatch.f90
* t02_kind_mismatch.f90
* t03_assumed_shape_buffer.f90
* t04_sendrecv_pair.f90

### Contiguity Tests

* t05_stride2_section.f90
* t06_2d_column_section.f90
* t07_assumed_shape_no_contiguous.f90
* t08_contiguous_attr_ok.f90

### Derived-Type Safety Tests

* t09_no_bind_c.f90
* t10_allocatable_component.f90
* t11_pointer_component.f90
* t12_correct_bind_c.f90
* t13_type_create_struct.f90

### Optional Argument Tests

* t14_optional_buf_no_present.f90
* t15_optional_status_dropped.f90
* t16_optional_correct_guard.f90

### Collective Communication Tests

* t17_collective_inside_if.f90
* t18_mismatched_branches.f90
* t19_correct_ordering.f90

### Wrapper / Advanced Cases

* t20_module_wrapper.f90
* t21_array_of_structs.f90
* t22_coarray_mixed.f90

---

## 4. Experimental Results

The complete test suite was executed using the provided run script.

### Command

```bash
./run.sh
```

### Result Summary

| Metric           | Value |
| ---------------- | ----- |
| Total test cases | 22    |
| Passed           | 22    |
| Failed           | 0     |
| Pass rate        | 100%  |

Observed output:

```text
=== Results: 22/22 OK, 0 failed ===
```

All expected diagnostics matched the expected outputs defined by the test suite.

---

## 5. Example Diagnostics

### Buffer Size Violation

The checker successfully identifies cases where the MPI count argument exceeds the size of the supplied buffer.

Rule triggered:

* BufferSizeRule

### Collective Ordering Violation

The checker detects collective MPI operations executed under inconsistent control flow.

Rule triggered:

* CollectiveOrderRule

### Derived-Type Safety Violation

The checker reports derived types containing unsupported components such as pointers and allocatables.

Rule triggered:

* DerivedTypeRule

### Non-Contiguous Array Section

The checker warns when potentially non-contiguous array sections are passed directly to MPI communication routines.

Rule triggered:

* ContiguityRule

---

## 6. Limitations

Current limitations include:

* Interprocedural analysis is limited.
* Dynamic runtime values cannot always be resolved statically.
* The checker currently targets MPI usage patterns represented in the provided test suite.
* Full MPI standard coverage is outside the current scope.

---

## 7. Conclusion

The proposed Flang MPI Checker successfully extends LLVM/Flang semantic analysis with MPI-specific correctness checks. Evaluation on a 22-case test suite achieved a 100% pass rate, demonstrating the ability to detect common MPI programming errors that are not reported by standard compilation alone.
