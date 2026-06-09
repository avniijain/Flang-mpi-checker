! BUG: REAL(4) buffer sent as MPI_DOUBLE_PRECISION (8 bytes/elem)
! EXPECTED: BufferSizeRule error
program t02_kind_mismatch
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
  integer      :: ierr, rank
  real(kind=4) :: data(100)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  data = 3.14_4
  if (rank == 0) then
    call MPI_Send(data, 100, MPI_DOUBLE_PRECISION, 1, 0, MPI_COMM_WORLD, ierr)
  else
    call MPI_Recv(data, 100, MPI_DOUBLE_PRECISION, 0, 0, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
