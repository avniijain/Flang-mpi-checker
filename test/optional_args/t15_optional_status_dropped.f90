! BUG: OPTIONAL status - errors silently suppressed when absent
! EXPECTED: OptionalArgRule warning
module t15_wrapper
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
contains
  subroutine safe_recv(buf, count, src, tag, comm, status)
    real(kind=8),      intent(out) :: buf(:)
    integer,           intent(in)  :: count, src, tag, comm
    integer, optional, intent(out) :: status(MPI_STATUS_SIZE)
    integer :: ierr_local
    if (present(status)) then
      call MPI_Recv(buf, count, MPI_DOUBLE_PRECISION, src, tag, comm, status, ierr_local)
    else
      call MPI_Recv(buf, count, MPI_DOUBLE_PRECISION, src, tag, comm, mpi_status_arr, ierr_local)
    end if
  end subroutine
end module
program t15_optional_status_dropped
  use mpi
  use t15_wrapper
  implicit none
  integer      :: ierr, rank
  real(kind=8) :: data(50)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    data = 2.0d0
    call MPI_Send(data, 50, MPI_DOUBLE_PRECISION, 1, 5, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call safe_recv(data, 50, 0, 5, MPI_COMM_WORLD)
  end if
  call MPI_Finalize(ierr)
end program
