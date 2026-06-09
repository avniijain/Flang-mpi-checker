! BUG: buf has 10 elements, count=20 -> buffer overflow
! EXPECTED: BufferSizeRule error
program t01_basic_mismatch
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
  integer :: ierr, rank
  integer, dimension(10) :: buf
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    call MPI_Send(buf, 20, MPI_INTEGER, 1, 42, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(buf, 20, MPI_INTEGER, 0, 42, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
