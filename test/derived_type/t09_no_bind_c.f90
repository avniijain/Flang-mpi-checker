! BUG: derived type without BIND(C) or SEQUENCE used as MPI buffer
! EXPECTED: DerivedTypeRule error
module t09_types
  implicit none
  integer :: mpi_status_arr(5)
  type :: particle_t
    real(kind=8) :: x, y, z, vx, vy, vz
    integer      :: id
  end type
end module
program t09_no_bind_c
  use mpi
  use t09_types
  implicit none
  integer          :: ierr, rank
  type(particle_t) :: particles(1000)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  if (rank == 0) then
    call MPI_Send(particles, 1000*storage_size(particles(1))/8, &
                  MPI_BYTE, 1, 0, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(particles, 1000*storage_size(particles(1))/8, &
                  MPI_BYTE, 0, 0, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
