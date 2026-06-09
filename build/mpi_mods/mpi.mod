!mod$ v1 sum:8d16e23610934278
module mpi
integer(4),parameter::mpi_comm_world=1_4
integer(4),parameter::mpi_integer=1_4
integer(4),parameter::mpi_real=2_4
integer(4),parameter::mpi_double_precision=3_4
integer(4),parameter::mpi_complex=4_4
integer(4),parameter::mpi_logical=5_4
integer(4),parameter::mpi_character=6_4
integer(4),parameter::mpi_byte=7_4
integer(4),parameter::mpi_sum=1_4
integer(4),parameter::mpi_max=2_4
integer(4),parameter::mpi_min=3_4
integer(4),parameter::mpi_prod=4_4
integer(4),parameter::mpi_status_ignore(1_8:5_8)=[INTEGER(4)::0_4,0_4,0_4,0_4,0_4]
integer(4),parameter::mpi_success=0_4
integer(4),parameter::mpi_any_source=-2_4
integer(4),parameter::mpi_any_tag=-1_4
integer(4),parameter::mpi_status_size=5_4
integer(4),parameter::mpi_address_kind=8_4
interface
subroutine mpi_init(ierr)
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_finalize(ierr)
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_comm_rank(comm,rank,ierr)
integer(4),intent(in)::comm
integer(4),intent(out)::rank
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_comm_size(comm,size,ierr)
integer(4),intent(in)::comm
integer(4),intent(out)::size
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_send(buf,count,datatype,dest,tag,comm,ierr)
type(*),intent(in)::buf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::dest
integer(4),intent(in)::tag
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_recv(buf,count,datatype,source,tag,comm,status,ierr)
type(*),intent(inout)::buf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::source
integer(4),intent(in)::tag
integer(4),intent(in)::comm
integer(4),intent(in)::status(1_8:*)
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_isend(buf,count,datatype,dest,tag,comm,request,ierr)
type(*),intent(in)::buf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::dest
integer(4),intent(in)::tag
integer(4),intent(in)::comm
integer(4),intent(out)::request
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_irecv(buf,count,datatype,source,tag,comm,request,ierr)
type(*),intent(inout)::buf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::source
integer(4),intent(in)::tag
integer(4),intent(in)::comm
integer(4),intent(out)::request
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_sendrecv(sendbuf,sendcount,sendtype,dest,sendtag,recvbuf,recvcount,recvtype,source,recvtag,comm,status,ierr)
type(*),intent(in)::sendbuf(1_8:*)
integer(4),intent(in)::sendcount
integer(4),intent(in)::sendtype
integer(4),intent(in)::dest
integer(4),intent(in)::sendtag
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::recvcount
integer(4),intent(in)::recvtype
integer(4),intent(in)::source
integer(4),intent(in)::recvtag
integer(4),intent(in)::comm
integer(4),intent(in)::status(1_8:*)
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_bcast(buf,count,datatype,root,comm,ierr)
type(*),intent(inout)::buf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::root
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_reduce(sendbuf,recvbuf,count,datatype,op,root,comm,ierr)
type(*),intent(in)::sendbuf(1_8:*)
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::op
integer(4),intent(in)::root
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_allreduce(sendbuf,recvbuf,count,datatype,op,comm,ierr)
type(*),intent(in)::sendbuf(1_8:*)
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::count
integer(4),intent(in)::datatype
integer(4),intent(in)::op
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_gather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm,ierr)
type(*),intent(in)::sendbuf(1_8:*)
integer(4),intent(in)::sendcount
integer(4),intent(in)::sendtype
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::recvcount
integer(4),intent(in)::recvtype
integer(4),intent(in)::root
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_scatter(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm,ierr)
type(*),intent(in)::sendbuf(1_8:*)
integer(4),intent(in)::sendcount
integer(4),intent(in)::sendtype
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::recvcount
integer(4),intent(in)::recvtype
integer(4),intent(in)::root
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_allgather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm,ierr)
type(*),intent(in)::sendbuf(1_8:*)
integer(4),intent(in)::sendcount
integer(4),intent(in)::sendtype
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::recvcount
integer(4),intent(in)::recvtype
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_alltoall(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm,ierr)
type(*),intent(in)::sendbuf(1_8:*)
integer(4),intent(in)::sendcount
integer(4),intent(in)::sendtype
type(*),intent(inout)::recvbuf(1_8:*)
integer(4),intent(in)::recvcount
integer(4),intent(in)::recvtype
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_barrier(comm,ierr)
integer(4),intent(in)::comm
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_type_create_struct(count,blocklengths,displacements,types,newtype,ierr)
integer(4),intent(in)::count
integer(4),intent(in)::blocklengths(1_8:*)
integer(8),intent(in)::displacements(1_8:*)
integer(4),intent(in)::types(1_8:*)
integer(4),intent(out)::newtype
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_type_commit(datatype,ierr)
integer(4),intent(inout)::datatype
integer(4),intent(out)::ierr
end
end interface
interface
subroutine mpi_type_free(datatype,ierr)
integer(4),intent(inout)::datatype
integer(4),intent(out)::ierr
end
end interface
end
