module mpi
  implicit none
  integer, parameter :: MPI_COMM_WORLD       = 1
  integer, parameter :: MPI_INTEGER          = 1
  integer, parameter :: MPI_REAL             = 2
  integer, parameter :: MPI_DOUBLE_PRECISION = 3
  integer, parameter :: MPI_COMPLEX          = 4
  integer, parameter :: MPI_LOGICAL          = 5
  integer, parameter :: MPI_CHARACTER        = 6
  integer, parameter :: MPI_BYTE             = 7
  integer, parameter :: MPI_SUM              = 1
  integer, parameter :: MPI_MAX              = 2
  integer, parameter :: MPI_MIN              = 3
  integer, parameter :: MPI_PROD             = 4
  integer, parameter :: MPI_STATUS_IGNORE(5) = [0,0,0,0,0]
  integer, parameter :: MPI_SUCCESS          = 0
  integer, parameter :: MPI_ANY_SOURCE       = -2
  integer, parameter :: MPI_ANY_TAG          = -1
  integer, parameter :: MPI_STATUS_SIZE      = 5
  integer, parameter :: MPI_ADDRESS_KIND     = 8
  interface
    subroutine MPI_Init(ierr)
      integer, intent(out) :: ierr
    end subroutine
    subroutine MPI_Finalize(ierr)
      integer, intent(out) :: ierr
    end subroutine
    subroutine MPI_Comm_rank(comm, rank, ierr)
      integer, intent(in)  :: comm
      integer, intent(out) :: rank, ierr
    end subroutine
    subroutine MPI_Comm_size(comm, size, ierr)
      integer, intent(in)  :: comm
      integer, intent(out) :: size, ierr
    end subroutine
    subroutine MPI_Send(buf, count, datatype, dest, tag, comm, ierr)
      type(*), intent(in)    :: buf(*)
      integer, intent(in)    :: count, datatype, dest, tag, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Recv(buf, count, datatype, source, tag, comm, status, ierr)
      type(*), intent(inout) :: buf(*)
      integer, intent(in)    :: count, datatype, source, tag, comm
      integer, intent(in)    :: status(*)
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Isend(buf, count, datatype, dest, tag, comm, request, ierr)
      type(*), intent(in)    :: buf(*)
      integer, intent(in)    :: count, datatype, dest, tag, comm
      integer, intent(out)   :: request, ierr
    end subroutine
    subroutine MPI_Irecv(buf, count, datatype, source, tag, comm, request, ierr)
      type(*), intent(inout) :: buf(*)
      integer, intent(in)    :: count, datatype, source, tag, comm
      integer, intent(out)   :: request, ierr
    end subroutine
    subroutine MPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, &
                             recvbuf, recvcount, recvtype, source, recvtag, &
                             comm, status, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: sendcount, sendtype, dest, sendtag
      integer, intent(in)    :: recvcount, recvtype, source, recvtag, comm
      integer, intent(in)    :: status(*)
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Bcast(buf, count, datatype, root, comm, ierr)
      type(*), intent(inout) :: buf(*)
      integer, intent(in)    :: count, datatype, root, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: count, datatype, op, root, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: count, datatype, op, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Gather(sendbuf, sendcount, sendtype, &
                          recvbuf, recvcount, recvtype, root, comm, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: sendcount, sendtype, recvcount, recvtype, root, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Scatter(sendbuf, sendcount, sendtype, &
                           recvbuf, recvcount, recvtype, root, comm, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: sendcount, sendtype, recvcount, recvtype, root, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Allgather(sendbuf, sendcount, sendtype, &
                             recvbuf, recvcount, recvtype, comm, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: sendcount, sendtype, recvcount, recvtype, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Alltoall(sendbuf, sendcount, sendtype, &
                            recvbuf, recvcount, recvtype, comm, ierr)
      type(*), intent(in)    :: sendbuf(*)
      type(*), intent(inout) :: recvbuf(*)
      integer, intent(in)    :: sendcount, sendtype, recvcount, recvtype, comm
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Barrier(comm, ierr)
      integer, intent(in)  :: comm
      integer, intent(out) :: ierr
    end subroutine
    subroutine MPI_Type_create_struct(count, blocklengths, displacements, &
                                      types, newtype, ierr)
      integer,         intent(in)  :: count, blocklengths(*), types(*)
      integer(kind=8), intent(in)  :: displacements(*)
      integer,         intent(out) :: newtype, ierr
    end subroutine
    subroutine MPI_Type_commit(datatype, ierr)
      integer, intent(inout) :: datatype
      integer, intent(out)   :: ierr
    end subroutine
    subroutine MPI_Type_free(datatype, ierr)
      integer, intent(inout) :: datatype
      integer, intent(out)   :: ierr
    end subroutine
  end interface
end module mpi
