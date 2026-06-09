! BUG: row section A(2,:) is non-contiguous in Fortran column-major order
! EXPECTED: ContiguityRule error for row section
program t06_2d_column_section
  use mpi
  implicit none
  integer :: mpi_status_arr(5)
  integer      :: ierr, rank
  real(kind=8) :: matrix(100, 50)
  real(kind=8) :: col_buf(100), row_buf(50)
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  matrix = 0.0d0
  if (rank == 0) then
    call MPI_Send(matrix(:, 3),  100, MPI_DOUBLE_PRECISION, 1, 1, MPI_COMM_WORLD, ierr)
    call MPI_Send(matrix(2, :),   50, MPI_DOUBLE_PRECISION, 1, 2, MPI_COMM_WORLD, ierr)
  else if (rank == 1) then
    call MPI_Recv(col_buf, 100, MPI_DOUBLE_PRECISION, 0, 1, MPI_COMM_WORLD, mpi_status_arr, ierr)
    call MPI_Recv(row_buf,  50, MPI_DOUBLE_PRECISION, 0, 2, MPI_COMM_WORLD, mpi_status_arr, ierr)
  end if
  call MPI_Finalize(ierr)
end program
