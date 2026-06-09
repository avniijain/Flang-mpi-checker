! BUG: A(1:200:2) stride=2, non-contiguous
! EXPECTED: ContiguityRule error
program t05_stride2_section
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
  integer :: ierr, rank
  real    :: A(200), B(100)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  A = 1.0
  if (rank == 0) then
    call MPI_Send(A(1:200:2), 100, MPI_REAL, 1, 0, MPI_COMM_WORLD, ierr)
  else
    call MPI_Recv(B, 100, MPI_REAL, 0, 0, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
