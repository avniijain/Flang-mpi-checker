! BUG: assumed-shape without CONTIGUOUS - caller may pass non-contiguous section
! EXPECTED: ContiguityRule warning
module t07_comm_mod
  use mpi
  implicit none
contains
  subroutine bcast_slice(data, root)
    real(kind=8), intent(inout) :: data(:)
    integer,      intent(in)    :: root
    integer :: ierr
    call MPI_Bcast(data, size(data), MPI_DOUBLE_PRECISION, root, MPI_COMM_WORLD, ierr)
  end subroutine
end module
program t07_assumed_shape_no_contiguous
  use mpi
  use t07_comm_mod
  implicit none
  integer      :: ierr
  real(kind=8) :: grid(1000, 1000)
  call MPI_Init(ierr)
  grid = 0.0d0
  call bcast_slice(grid(5, :), 0)
  call MPI_Finalize(ierr)
end program
