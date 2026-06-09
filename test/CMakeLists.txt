find_program(LLVM_LIT NAMES llvm-lit lit)
if(NOT LLVM_LIT)
  message(WARNING "llvm-lit not found - tests registered but cannot auto-run.")
endif()

function(add_mpi_check_test test_name fortran_src)
  add_test(
    NAME "${test_name}"
    COMMAND ${CMAKE_BINARY_DIR}/tools/flang-mpi-checker/flang-mpi-checker
            ${CMAKE_CURRENT_SOURCE_DIR}/${fortran_src}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endfunction()

add_mpi_check_test(buf_mismatch_basic          buffer_mismatch/t01_basic_mismatch.f90)
add_mpi_check_test(buf_mismatch_kind           buffer_mismatch/t02_kind_mismatch.f90)
add_mpi_check_test(buf_mismatch_assumed_shape  buffer_mismatch/t03_assumed_shape_buffer.f90)
add_mpi_check_test(buf_mismatch_sendrecv       buffer_mismatch/t04_sendrecv_pair.f90)
add_mpi_check_test(noncontig_stride2           noncontiguous/t05_stride2_section.f90)
add_mpi_check_test(noncontig_2d_column         noncontiguous/t06_2d_column_section.f90)
add_mpi_check_test(noncontig_assumed_shape_nc  noncontiguous/t07_assumed_shape_no_contiguous.f90)
add_mpi_check_test(noncontig_contiguous_ok     noncontiguous/t08_contiguous_attr_ok.f90)
add_mpi_check_test(dtype_no_bind_c             derived_type/t09_no_bind_c.f90)
add_mpi_check_test(dtype_allocatable           derived_type/t10_allocatable_component.f90)
add_mpi_check_test(dtype_pointer               derived_type/t11_pointer_component.f90)
add_mpi_check_test(dtype_correct_bind_c        derived_type/t12_correct_bind_c.f90)
add_mpi_check_test(dtype_struct_offset         derived_type/t13_type_create_struct.f90)
add_mpi_check_test(optional_buf_no_present     optional_args/t14_optional_buf_no_present.f90)
add_mpi_check_test(optional_status_dropped     optional_args/t15_optional_status_dropped.f90)
add_mpi_check_test(optional_correct_guard      optional_args/t16_optional_correct_guard.f90)
add_mpi_check_test(coll_inside_if              collective_ordering/t17_collective_inside_if.f90)
add_mpi_check_test(coll_mismatched_branches    collective_ordering/t18_mismatched_branches.f90)
add_mpi_check_test(coll_correct_order          collective_ordering/t19_correct_ordering.f90)
add_mpi_check_test(fortran_module_wrapper      fortran_specific/t20_module_wrapper.f90)
add_mpi_check_test(fortran_array_of_structs    fortran_specific/t21_array_of_structs.f90)
add_mpi_check_test(fortran_coarray_mixed       fortran_specific/t22_coarray_mixed.f90)
