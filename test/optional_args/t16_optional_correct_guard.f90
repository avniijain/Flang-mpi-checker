! NEGATIVE TEST: PRESENT() guard in place - no warnings expected
module t16_wrapper
  use mpi
  implicit none
contains
  subroutine maybe_send(buf, count, dest, tag, comm, ierr)
    real(kind=8), optional, intent(in) :: buf(:)
    integer,                intent(in)  :: count, dest, tag, comm
    integer,                intent(out) :: ierr
    if (present(buf)) then
      call MPI_Send(buf, count, MPI_DOUBLE_PRECISION, dest, tag, comm, ierr)
    else
      ierr = MPI_SUCCESS
    end if
  end subroutine
end module
program t16_optional_correct_guard
  use mpi
  use t16_wrapper
  implicit none
  integer      :: ierr, rank
  real(kind=8) :: data(64)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    data = 7.0d0
    call maybe_send(data, 64, 1, 3, MPI_COMM_WORLD, ierr)
  else
    call maybe_send(count=0, dest=0, tag=3, comm=MPI_COMM_WORLD, ierr=ierr)
  end if
  call MPI_Finalize(ierr)
end program
