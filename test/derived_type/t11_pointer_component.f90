! BUG: POINTER component - address is rank-local, meaningless on receiver
! EXPECTED: DerivedTypeRule error
module t11_types
  implicit none
  integer :: mpi_status_arr(5)
  type :: linked_node_t
    real(kind=8)                 :: value
    type(linked_node_t), pointer :: next => null()
  end type
end module
program t11_pointer_component
  use mpi
  use t11_types
  implicit none
  integer             :: ierr, rank
  type(linked_node_t) :: node
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  node%value = 42.0d0
  node%next  => null()
  if (rank == 0) then
    call MPI_Send(node, storage_size(node)/8, MPI_BYTE, 1, 0, MPI_COMM_WORLD, ierr)
  else
    call MPI_Recv(node, storage_size(node)/8, MPI_BYTE, 0, 0, &
                  MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
