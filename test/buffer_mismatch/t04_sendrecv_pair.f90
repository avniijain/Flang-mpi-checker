! BUG: send 50 doubles, recv buffer only 20
! EXPECTED: BufferSizeRule error
program t04_sendrecv_pair
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
  integer      :: ierr, rank
  real(kind=8) :: send_buf(50), recv_buf(20)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  send_buf = 1.0d0
  if (rank == 0) then
    call MPI_Send(send_buf, 50, MPI_DOUBLE_PRECISION, 1, 7, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(recv_buf, 20, MPI_DOUBLE_PRECISION, 0, 7, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
