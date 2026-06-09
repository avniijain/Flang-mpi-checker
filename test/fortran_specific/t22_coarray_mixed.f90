! BUG: SYNC ALL + MPI_Barrier interleave; coarray as MPI buffer
! EXPECTED: CollectiveOrderRule warning + ContiguityRule warning
program t22_coarray_mixed
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
  integer      :: ierr, rank
  real(kind=8) :: shared_data[*]
  real(kind=8) :: mpi_buf(1)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  shared_data = real(rank, 8)
  sync all
  call MPI_Barrier(MPI_COMM_WORLD, ierr)
  mpi_buf(1) = shared_data
  if (rank == 0) then
    call MPI_Send(shared_data, 1, MPI_DOUBLE_PRECISION, 1, 0, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(mpi_buf, 1, MPI_DOUBLE_PRECISION, 0, 0, &
                  MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
