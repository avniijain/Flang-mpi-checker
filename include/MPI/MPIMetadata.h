#pragma once
// LLVM 18 removed llvm::Optional - use std::optional throughout
#include "flang/Semantics/symbol.h"
#include "flang/Semantics/type.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SourceMgr.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace flang_mpi_checker {

enum class ContiguityKind { Contiguous, NonContiguous, MaybeContiguous, Unknown };
llvm::StringRef toString(ContiguityKind k);

struct ArrayDescriptor {
  int rank{0};
  std::optional<int64_t> elementBytes;
  struct DimInfo { std::optional<int64_t> lower, upper, stride, extent; };
  llvm::SmallVector<DimInfo, 7> dims;
  bool hasCONTIGUOUSAttr{false}, isAssumedShape{false}, isAssumedSize{false}, isSection{false};
  ContiguityKind contiguity{ContiguityKind::Unknown};
  std::optional<int64_t> totalElements() const;
  std::optional<int64_t> totalBytes() const;
};

struct DerivedTypeLayout {
  std::string typeName;
  bool hasBindC{false}, hasSequence{false}, hasAllocatableComponent{false}, hasPointerComponent{false};
  struct ComponentInfo {
    std::string name, typeName;
    int64_t offsetBytes{-1}, sizeBytes{-1};
    bool isAllocatable{false}, isPointer{false};
  };
  std::vector<ComponentInfo> components;
  bool isMPISafe() const;
};

enum class MPICallKind {
  Send, Recv, Isend, Irecv, Sendrecv,
  Reduce, Gather, Gatherv, Bcast, Scatter, Scatterv,
  Allreduce, Allgather, Allgatherv, Alltoall, Alltoallv,
  Barrier, TypeCreateStruct, TypeCommit, TypeFree,
  Init, Finalize, CommRank, CommSize, Unknown
};
llvm::StringRef toString(MPICallKind k);
MPICallKind mpiCallKindFromName(llvm::StringRef name);
bool isCollective(MPICallKind k);
bool isPointToPoint(MPICallKind k);

struct OptionalArgInfo {
  std::string argName;
  bool isDeclaredOptional{false}, isPresentAtCallSite{false}, mayBeAbsent{false};
};

struct MPICallMetadata {
  MPICallKind kind{MPICallKind::Unknown};
  std::string calleeName;
  llvm::SMLoc location;
  std::optional<ArrayDescriptor>   buffer;
  std::optional<int64_t>           count;
  std::string                      mpiDatatype;
  std::optional<int64_t>           datatypeBytes;
  std::optional<int64_t> envelopeBytes() const {
    if (count && datatypeBytes) return *count * *datatypeBytes;
    return std::nullopt;
  }
  std::optional<DerivedTypeLayout> derivedTypeLayout;
  std::string                      communicator{"mpi_comm_world"};
  std::optional<int>               tag, dest, source;
  std::vector<OptionalArgInfo>     optionalArgs;
  bool insideConditional{false}, insideLoop{false};
  int  nestingDepth{0}, collectiveSeq{-1};
};

struct CollectiveCall {
  MPICallKind kind{MPICallKind::Unknown};
  std::string communicator, procedureName;
  llvm::SMLoc location;
  bool        insideConditional{false};
  int         seqInScope{0};
};

} // namespace flang_mpi_checker
