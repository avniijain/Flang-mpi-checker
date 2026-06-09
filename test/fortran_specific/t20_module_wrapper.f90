! BUG: INTEGER buf sent as MPI_REAL in wrong overload of generic interface
! EXPECTED: BufferSizeRule error (datatype mismatch)
module t20_comm_wrappers
  use mpi
  implicit none
  interface send_data
    module procedure send_integer, send_real8
  end interface
contains
  subroutine send_integer(buf, n, dest, tag)
    integer, intent(in) :: buf(:), n, dest, tag
    integer :: ierr
    call MPI_Send(buf, n, MPI_REAL, dest, tag, MPI_COMM_WORLD, ierr)
  end subroutine
  subroutine send_real8(buf, n, dest, tag)
    real(kind=8), intent(in) :: buf(:)
    integer,      intent(in) :: n, dest, tag
    integer :: ierr
    call MPI_Send(buf, n, MPI_DOUBLE_PRECISION, dest, tag, MPI_COMM_WORLD, ierr)
  end subroutine
end module
program t20_module_wrapper
  use mpi
  use t20_comm_wrappers
  implicit none
  integer      :: ierr, rank
  integer      :: int_data(50)
  real(kind=8) :: real_data(50)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    int_data  = [(i, i=1,50)]
    real_data = real(int_data, 8)
    call send_data(int_data,  50, 1, 0)
    call send_data(real_data, 50, 1, 1)
  end if
  call MPI_Finalize(ierr)
end program
