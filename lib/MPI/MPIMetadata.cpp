#include "MPI/MPIMetadata.h"
#include "llvm/ADT/StringSwitch.h"
#include <algorithm>
#include <numeric>

namespace flang_mpi_checker {

llvm::StringRef toString(ContiguityKind k) {
  switch (k) {
  case ContiguityKind::Contiguous:      return "Contiguous";
  case ContiguityKind::NonContiguous:   return "NonContiguous";
  case ContiguityKind::MaybeContiguous: return "MaybeContiguous";
  default:                              return "Unknown";
  }
}

std::optional<int64_t> ArrayDescriptor::totalElements() const {
  if (dims.empty()) return 1;
  int64_t total = 1;
  for (const auto &d : dims) {
    if (!d.extent || *d.extent < 0) return std::nullopt;
    total *= *d.extent;
  }
  return total;
}

std::optional<int64_t> ArrayDescriptor::totalBytes() const {
  if (!elementBytes) return std::nullopt;
  auto elems = totalElements();
  if (!elems) return std::nullopt;
  return *elems * *elementBytes;
}

bool DerivedTypeLayout::isMPISafe() const {
  if (!hasBindC && !hasSequence) return false;
  if (hasAllocatableComponent || hasPointerComponent) return false;
  return true;
}

llvm::StringRef toString(MPICallKind k) {
  switch (k) {
  case MPICallKind::Send:             return "Send";
  case MPICallKind::Recv:             return "Recv";
  case MPICallKind::Isend:            return "Isend";
  case MPICallKind::Irecv:            return "Irecv";
  case MPICallKind::Sendrecv:         return "Sendrecv";
  case MPICallKind::Reduce:           return "Reduce";
  case MPICallKind::Gather:           return "Gather";
  case MPICallKind::Gatherv:          return "Gatherv";
  case MPICallKind::Bcast:            return "Bcast";
  case MPICallKind::Scatter:          return "Scatter";
  case MPICallKind::Scatterv:         return "Scatterv";
  case MPICallKind::Allreduce:        return "Allreduce";
  case MPICallKind::Allgather:        return "Allgather";
  case MPICallKind::Allgatherv:       return "Allgatherv";
  case MPICallKind::Alltoall:         return "Alltoall";
  case MPICallKind::Alltoallv:        return "Alltoallv";
  case MPICallKind::Barrier:          return "Barrier";
  case MPICallKind::TypeCreateStruct: return "TypeCreateStruct";
  case MPICallKind::TypeCommit:       return "TypeCommit";
  case MPICallKind::TypeFree:         return "TypeFree";
  case MPICallKind::Init:             return "Init";
  case MPICallKind::Finalize:         return "Finalize";
  case MPICallKind::CommRank:         return "CommRank";
  case MPICallKind::CommSize:         return "CommSize";
  default:                            return "Unknown";
  }
}

MPICallKind mpiCallKindFromName(llvm::StringRef name) {
  return llvm::StringSwitch<MPICallKind>(name.lower())
    .Case("mpi_send",               MPICallKind::Send)
    .Case("mpi_recv",               MPICallKind::Recv)
    .Case("mpi_isend",              MPICallKind::Isend)
    .Case("mpi_irecv",              MPICallKind::Irecv)
    .Case("mpi_sendrecv",           MPICallKind::Sendrecv)
    .Case("mpi_reduce",             MPICallKind::Reduce)
    .Case("mpi_gather",             MPICallKind::Gather)
    .Case("mpi_gatherv",            MPICallKind::Gatherv)
    .Case("mpi_bcast",              MPICallKind::Bcast)
    .Case("mpi_scatter",            MPICallKind::Scatter)
    .Case("mpi_scatterv",           MPICallKind::Scatterv)
    .Case("mpi_allreduce",          MPICallKind::Allreduce)
    .Case("mpi_allgather",          MPICallKind::Allgather)
    .Case("mpi_allgatherv",         MPICallKind::Allgatherv)
    .Case("mpi_alltoall",           MPICallKind::Alltoall)
    .Case("mpi_alltoallv",          MPICallKind::Alltoallv)
    .Case("mpi_barrier",            MPICallKind::Barrier)
    .Case("mpi_type_create_struct", MPICallKind::TypeCreateStruct)
    .Case("mpi_type_commit",        MPICallKind::TypeCommit)
    .Case("mpi_type_free",          MPICallKind::TypeFree)
    .Case("mpi_init",               MPICallKind::Init)
    .Case("mpi_finalize",           MPICallKind::Finalize)
    .Case("mpi_comm_rank",          MPICallKind::CommRank)
    .Case("mpi_comm_size",          MPICallKind::CommSize)
    .Default(MPICallKind::Unknown);
}

bool isCollective(MPICallKind k) {
  switch (k) {
  case MPICallKind::Reduce:    case MPICallKind::Gather:
  case MPICallKind::Gatherv:   case MPICallKind::Bcast:
  case MPICallKind::Scatter:   case MPICallKind::Scatterv:
  case MPICallKind::Allreduce: case MPICallKind::Allgather:
  case MPICallKind::Allgatherv:case MPICallKind::Alltoall:
  case MPICallKind::Alltoallv: case MPICallKind::Barrier:
    return true;
  default: return false;
  }
}

bool isPointToPoint(MPICallKind k) {
  switch (k) {
  case MPICallKind::Send:  case MPICallKind::Recv:
  case MPICallKind::Isend: case MPICallKind::Irecv:
  case MPICallKind::Sendrecv:
    return true;
  default: return false;
  }
}

} // namespace flang_mpi_checker
