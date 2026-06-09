! BUG: hard-coded offsets in MPI_Type_create_struct - may be wrong
! EXPECTED: DerivedTypeRule warning
! Note: SEQUENCE goes inside the type body, not on the TYPE statement
module t13_types
  implicit none
  integer :: mpi_status_arr(5)
  type :: particle_t
    sequence
    real(8)  :: mass, charge
    integer  :: id, pad
  end type
end module
program t13_type_create_struct
  use mpi
  use t13_types
  implicit none
  integer                   :: ierr, rank, mpi_particle_type
  integer                   :: blocklengths(3)
  integer(kind=MPI_ADDRESS_KIND) :: displacements(3)
  integer                   :: types(3)
  type(particle_t)          :: p
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  blocklengths  = [1, 1, 1]
  types         = [MPI_DOUBLE_PRECISION, MPI_DOUBLE_PRECISION, MPI_INTEGER]
  displacements = [0_MPI_ADDRESS_KIND, 8_MPI_ADDRESS_KIND, 20_MPI_ADDRESS_KIND]
  call MPI_Type_create_struct(3, blocklengths, displacements, types, mpi_particle_type, ierr)
  call MPI_Type_commit(mpi_particle_type, ierr)
  if (rank == 0) then
    p = particle_t(1.67d-27, -1.6d-19, 1, 0)
    call MPI_Send(p, 1, mpi_particle_type, 1, 0, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(p, 1, mpi_particle_type, 0, 0, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Type_free(mpi_particle_type, ierr)
  call MPI_Finalize(ierr)
end program
