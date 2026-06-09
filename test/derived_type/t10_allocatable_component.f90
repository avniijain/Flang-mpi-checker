! BUG: ALLOCATABLE component - MPI sends heap pointer, not data
! EXPECTED: DerivedTypeRule error
module t10_types
  implicit none
  integer :: mpi_status_arr(5)
  type :: sparse_vec_t
    integer                   :: nnz
    real(kind=8), allocatable :: values(:)
    integer,      allocatable :: indices(:)
  end type
end module
program t10_allocatable_component
  use mpi
  use t10_types
  implicit none
  integer            :: ierr, rank
  type(sparse_vec_t) :: vec
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    vec%nnz = 5
    allocate(vec%values(5), vec%indices(5))
    vec%values  = [1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0]
    vec%indices = [1, 3, 7, 12, 20]
    call MPI_Send(vec, storage_size(vec)/8, MPI_BYTE, 1, 0, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(vec, storage_size(vec)/8, MPI_BYTE, 0, 0, &
                  MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
