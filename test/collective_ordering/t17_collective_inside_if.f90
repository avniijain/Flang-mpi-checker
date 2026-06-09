! BUG: MPI_Bcast inside rank-0-only IF block - deadlock
! EXPECTED: CollectiveOrderRule warning
program t17_collective_inside_if
  use mpi
  implicit none
  integer      :: ierr, rank
  real(kind=8) :: value
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  value = 0.0d0
  if (rank == 0) then
    value = 42.0d0
    call MPI_Bcast(value, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
  end if
  call MPI_Finalize(ierr)
end program
