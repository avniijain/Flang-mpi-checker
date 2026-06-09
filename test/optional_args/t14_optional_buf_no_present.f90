! BUG: OPTIONAL buffer passed to MPI without PRESENT() guard
! EXPECTED: OptionalArgRule error
module t14_wrapper
  use mpi
  implicit none
contains
  subroutine conditional_send(buf, count, dest, tag, comm, ierr)
    real(kind=8), optional, intent(in) :: buf(:)
    integer,                intent(in)  :: count, dest, tag, comm
    integer,                intent(out) :: ierr
    call MPI_Send(buf, count, MPI_DOUBLE_PRECISION, dest, tag, comm, ierr)
  end subroutine
end module
program t14_optional_buf_no_present
  use mpi
  use t14_wrapper
  implicit none
  integer      :: ierr, rank
  real(kind=8) :: data(100)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    data = 1.0d0
    call conditional_send(data, 100, 1, 0, MPI_COMM_WORLD, ierr)
  else
    call conditional_send(count=0, dest=0, tag=0, comm=MPI_COMM_WORLD, ierr=ierr)
  end if
  call MPI_Finalize(ierr)
end program
