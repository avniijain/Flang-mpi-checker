! BUG: assumed-shape dummy, count may exceed actual size
! EXPECTED: BufferSizeRule warning
module t03_mod
  use mpi
  implicit none
contains
  subroutine send_data(buf, count, dest)
    real(kind=8), intent(in) :: buf(:)
    integer,      intent(in) :: count, dest
    integer                  :: ierr
    call MPI_Send(buf, count, MPI_DOUBLE_PRECISION, dest, 99, MPI_COMM_WORLD, ierr)
  end subroutine
end module
program t03_assumed_shape_buffer
  use mpi
  use t03_mod
  implicit none
  integer      :: ierr, rank
  real(kind=8) :: small_buf(5)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) call send_data(small_buf, 100, 1)
  call MPI_Finalize(ierr)
end program
