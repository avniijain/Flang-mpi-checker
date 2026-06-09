! BUG: even ranks call Bcast, odd ranks call Reduce - deadlock
! EXPECTED: CollectiveOrderRule error
program t18_mismatched_branches
  use mpi
  implicit none
  integer      :: ierr, rank
  real(kind=8) :: val, result
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  val = real(rank, 8)
  if (mod(rank, 2) == 0) then
    call MPI_Bcast(val, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
  else
    call MPI_Reduce(val, 1, MPI_DOUBLE_PRECISION, MPI_SUM, 0, MPI_COMM_WORLD, ierr)
  end if
  call MPI_Finalize(ierr)
end program
