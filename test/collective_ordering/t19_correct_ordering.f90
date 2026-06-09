! NEGATIVE TEST: all collectives top-level, all ranks participate
program t19_correct_ordering
  use mpi
  implicit none
  integer      :: ierr, rank, nprocs
  real(kind=8) :: local_sum, global_sum, shared_value
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank,   ierr)
  call MPI_Comm_size(MPI_COMM_WORLD, nprocs, ierr)
  local_sum = real(rank * rank, 8)
  call MPI_Reduce(local_sum, global_sum, 1, MPI_DOUBLE_PRECISION, MPI_SUM, 0, MPI_COMM_WORLD, ierr)
  shared_value = global_sum
  call MPI_Bcast(shared_value, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
  call MPI_Barrier(MPI_COMM_WORLD, ierr)
  if (rank == 0) print *, "Sum of squares = ", shared_value
  call MPI_Finalize(ierr)
end program
