! BUG: no SEQUENCE on struct used in array MPI buffer
! EXPECTED: DerivedTypeRule error
module t21_types
  implicit none
  integer :: mpi_status_arr(5)
  type :: cell_t
    real(kind=4) :: temperature, pressure
    integer      :: material_id
  end type
end module
program t21_array_of_structs
  use mpi
  use t21_types
  implicit none
  integer        :: ierr, rank
  type(cell_t)   :: grid(256, 256)
  integer(kind=8):: byte_count
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  grid = cell_t(300.0, 101325.0, 1)
  byte_count = int(storage_size(grid(1,1)), 8) / 8 * 256 * 256
  if (rank == 0) then
    call MPI_Send(grid, int(byte_count), MPI_BYTE, 1, 0, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(grid, int(byte_count), MPI_BYTE, 0, 0, &
                  MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
