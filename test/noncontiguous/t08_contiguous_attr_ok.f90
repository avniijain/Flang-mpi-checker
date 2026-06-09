! NEGATIVE TEST: CONTIGUOUS declared - no warnings expected
module t08_mod
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
contains
  subroutine exchange(send_buf, recv_buf, dest, src, tag)
    real(kind=8), intent(in),    contiguous :: send_buf(:)
    real(kind=8), intent(inout), contiguous :: recv_buf(:)
    integer,      intent(in)                :: dest, src, tag
    integer :: ierr
    call MPI_Sendrecv(send_buf, size(send_buf), MPI_DOUBLE_PRECISION, dest, tag, &
                      recv_buf, size(recv_buf), MPI_DOUBLE_PRECISION, src,  tag, &
                      MPI_COMM_WORLD, mpi_status_arr, ierr)
  end subroutine
end module
program t08_contiguous_attr_ok
  use mpi
  use t08_mod
  implicit none
  integer      :: ierr, rank, nprocs
  real(kind=8) :: left(128), right(128)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank,   ierr)
  call MPI_Comm_size(MPI_COMM_WORLD, nprocs, ierr)
  left = real(rank, 8)
  right = 0.0d0
  call exchange(left, right, mod(rank+1,nprocs), mod(rank-1+nprocs,nprocs), 0)
  call MPI_Finalize(ierr)
end program
