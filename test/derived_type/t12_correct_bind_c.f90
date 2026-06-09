! NEGATIVE TEST: BIND(C) type - no DerivedTypeRule warnings expected
module t12_types
  use iso_c_binding
  implicit none
  integer :: mpi_status_arr(5)
  type, bind(c) :: vec3_t
    real(c_double) :: x, y, z
  end type
end module
program t12_correct_bind_c
  use mpi
  use t12_types
  implicit none
  integer      :: ierr, rank
  type(vec3_t) :: positions(512)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  positions = vec3_t(0.0d0, 0.0d0, 0.0d0)
  if (rank == 0) then
    positions(1) = vec3_t(1.0d0, 2.0d0, 3.0d0)
    call MPI_Send(positions, 512*3*8, MPI_BYTE, 1, 0, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(positions, 512*3*8, MPI_BYTE, 0, 0, &
                  MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
