#include "Analysis/CorrectnessRules.h"
#include "llvm/Support/FormatVariadic.h"
#include <algorithm>

namespace flang_mpi_checker {

// ── Rule 1: Buffer size ─────────────────────────────────────────────────────

void BufferSizeRule::check(const MPICallMetadata &call) {
  checkBufferEnvelope(call);
}

void BufferSizeRule::checkAll(
    const llvm::SmallVector<MPICallMetadata,64> &calls,
    const llvm::SmallVector<CollectiveCall,32> &) {
  matchSendRecvPairs(calls);
}

void BufferSizeRule::checkBufferEnvelope(const MPICallMetadata &call) {
  if (!call.buffer || !call.count || !call.datatypeBytes) return;
  const ArrayDescriptor &buf = *call.buffer;
  int64_t envBytes = *call.count * *call.datatypeBytes;

  auto bufBytes = buf.totalBytes();
  if (!bufBytes) {
    if (buf.isAssumedShape) {
      diag_.warning("BufferSizeRule",
          llvm::formatv("MPI call '{0}': buffer is assumed-shape — "
            "cannot verify it holds {1} byte(s) ({2} x {3}).",
            call.calleeName, envBytes, *call.count, *call.datatypeBytes).str(),
          call.location,
          "Add CONTIGUOUS attribute or explicit SIZE check.");
    }
    return;
  }
  if (*bufBytes < envBytes) {
    diag_.error("BufferSizeRule",
        llvm::formatv("MPI call '{0}': buffer has {1} byte(s) but envelope "
          "requires {2} byte(s) ({3} x {4}). Buffer overflow.",
          call.calleeName, *bufBytes, envBytes,
          *call.count, *call.datatypeBytes).str(),
        call.location,
        llvm::formatv("Resize buffer to at least {0} elements or "
          "reduce count to {1}.",
          *call.count, *bufBytes / *call.datatypeBytes).str());
  }
}

void BufferSizeRule::matchSendRecvPairs(
    const llvm::SmallVector<MPICallMetadata,64> &calls) {
  std::unordered_map<std::string, const MPICallMetadata*> sends;
  for (const auto &c : calls) {
    if ((c.kind!=MPICallKind::Send&&c.kind!=MPICallKind::Isend)) continue;
    if (!c.envelopeBytes()||!c.tag) continue;
    sends[c.communicator+"_"+std::to_string(*c.tag)] = &c;
  }
  for (const auto &c : calls) {
    if ((c.kind!=MPICallKind::Recv&&c.kind!=MPICallKind::Irecv)) continue;
    if (!c.envelopeBytes()||!c.tag) continue;
    auto it = sends.find(c.communicator+"_"+std::to_string(*c.tag));
    if (it==sends.end()) continue;
    int64_t snd = *it->second->envelopeBytes();
    int64_t rcv = *c.envelopeBytes();
    if (snd > rcv) {
      diag_.error("BufferSizeRule",
          llvm::formatv("Send/Recv mismatch on comm '{0}' tag {1}: "
            "sender sends {2} bytes, receiver buffer only {3} bytes.",
            c.communicator, *c.tag, snd, rcv).str(),
          c.location,
          llvm::formatv("Increase recv count from {0} to {1}.",
            *c.count, *it->second->count).str());
    }
  }
}

// ── Rule 2: Contiguity ──────────────────────────────────────────────────────

void ContiguityRule::check(const MPICallMetadata &call) {
  warnIfNonContiguous(call);
  warnAssumedShape(call);
}

void ContiguityRule::warnIfNonContiguous(const MPICallMetadata &call) {
  if (!call.buffer||!call.buffer->isSection) return;
  switch (call.buffer->contiguity) {
  case ContiguityKind::NonContiguous:
    diag_.error("ContiguityRule",
        llvm::formatv("MPI call '{0}': non-contiguous array section passed "
          "as buffer (stride != 1). MPI reads raw memory — data will be wrong.",
          call.calleeName).str(),
        call.location,
        "Copy section into a contiguous temp array, call MPI, copy back.");
    break;
  case ContiguityKind::MaybeContiguous:
    diag_.warning("ContiguityRule",
        llvm::formatv("MPI call '{0}': array section contiguity unknown at "
          "compile time. If stride != 1 at runtime, behaviour is undefined.",
          call.calleeName).str(),
        call.location,
        "Add IS_CONTIGUOUS() check or use a CONTIGUOUS temporary.");
    break;
  default: break;
  }
}

void ContiguityRule::warnAssumedShape(const MPICallMetadata &call) {
  if (!call.buffer||!call.buffer->isAssumedShape) return;
  if (!call.buffer->hasCONTIGUOUSAttr) {
    diag_.warning("ContiguityRule",
        llvm::formatv("MPI call '{0}': assumed-shape dummy without CONTIGUOUS "
          "attribute. Caller may pass a non-contiguous section.",
          call.calleeName).str(),
        call.location,
        "Declare dummy argument CONTIGUOUS, or check IS_CONTIGUOUS() at runtime.");
  }
}

// ── Rule 3: Derived type ────────────────────────────────────────────────────

void DerivedTypeRule::check(const MPICallMetadata &call) {
  if (call.kind==MPICallKind::TypeCreateStruct && !call.derivedTypeLayout) {
    diag_.warning("DerivedTypeRule",
        "MPI_Type_create_struct uses explicit displacements that cannot be "
        "validated statically against a Fortran derived type.",
        call.location,
        "Use MPI_Get_address/C_LOC to compute offsets at runtime.");
    return;
  }
  if (!call.derivedTypeLayout) return;
  checkBindCOrSequence(call);
  checkNoAllocatablePointer(call);
  if (call.kind==MPICallKind::TypeCreateStruct) checkStructOffsets(call);
}

void DerivedTypeRule::checkBindCOrSequence(const MPICallMetadata &call) {
  const auto &layout = *call.derivedTypeLayout;
  if (!layout.hasBindC&&!layout.hasSequence)
    diag_.error("DerivedTypeRule",
        llvm::formatv("MPI call '{0}': type '{1}' lacks BIND(C) or SEQUENCE. "
          "Memory layout is implementation-defined.",
          call.calleeName, layout.typeName).str(),
        call.location,
        llvm::formatv("Add SEQUENCE or BIND(C) to TYPE '{0}'.",
          layout.typeName).str());
}

void DerivedTypeRule::checkNoAllocatablePointer(const MPICallMetadata &call) {
  const auto &layout = *call.derivedTypeLayout;
  if (layout.hasAllocatableComponent)
    diag_.error("DerivedTypeRule",
        llvm::formatv("MPI call '{0}': type '{1}' has ALLOCATABLE components. "
          "MPI transmits the descriptor pointer, not the heap data.",
          call.calleeName, layout.typeName).str(),
        call.location, "Pack scalar fields into a SEQUENCE/BIND(C) type.");
  if (layout.hasPointerComponent)
    diag_.error("DerivedTypeRule",
        llvm::formatv("MPI call '{0}': type '{1}' has POINTER components. "
          "Pointers are rank-local addresses — meaningless on receiver.",
          call.calleeName, layout.typeName).str(),
        call.location, "Dereference and send the pointed-to data instead.");
}

void DerivedTypeRule::checkStructOffsets(const MPICallMetadata &call) {
  const auto &layout = *call.derivedTypeLayout;
  for (const auto &comp : layout.components) {
    if (comp.offsetBytes < 0) {
      diag_.warning("DerivedTypeRule",
          llvm::formatv("MPI_Type_create_struct for '{0}': component offsets "
            "unknown statically. Hard-coded displacements may be wrong.",
            layout.typeName).str(),
          call.location,
          "Use MPI_Get_address/C_LOC to compute offsets at runtime.");
      break;
    }
  }
}

// ── Rule 4: OPTIONAL args ───────────────────────────────────────────────────

void OptionalArgRule::check(const MPICallMetadata &call) {
  checkOptionalWithoutPresent(call);
  checkStatusOptional(call);
}

void OptionalArgRule::checkOptionalWithoutPresent(const MPICallMetadata &call) {
  static const char* critical[] = {
    "buf","buffer","sendbuf","recvbuf","count","sendcount","recvcount","datatype"
  };
  for (const auto &oa : call.optionalArgs) {
    if (!oa.isDeclaredOptional||!oa.mayBeAbsent) continue;
    for (const char *n : critical) {
      if (oa.argName.find(n)!=std::string::npos) {
        diag_.error("OptionalArgRule",
            llvm::formatv("MPI call '{0}': OPTIONAL argument '{1}' may be "
              "absent but is passed unconditionally — null dereference in MPI.",
              call.calleeName, oa.argName).str(),
            call.location,
            llvm::formatv("Guard with: IF (PRESENT({0})) THEN ... END IF",
              oa.argName).str());
        break;
      }
    }
  }
}

void OptionalArgRule::checkStatusOptional(const MPICallMetadata &call) {
  for (const auto &oa : call.optionalArgs) {
    if (oa.argName.find("status")==std::string::npos) continue;
    if (oa.isDeclaredOptional)
      diag_.warning("OptionalArgRule",
          llvm::formatv("MPI call '{0}': OPTIONAL status '{1}' may be absent "
            "— MPI errors will be silently suppressed.",
            call.calleeName, oa.argName).str(),
          call.location,
          "Pass MPI_STATUS_IGNORE explicitly or always check ierr.");
  }
}

// ── Rule 5: Collective ordering ─────────────────────────────────────────────

void CollectiveOrderRule::checkAll(
    const llvm::SmallVector<MPICallMetadata,64> &,
    const llvm::SmallVector<CollectiveCall,32>  &collectives) {
  checkConditionalCollective(collectives);
  checkOrderConsistency(collectives);
}

void CollectiveOrderRule::checkConditionalCollective(
    const llvm::SmallVector<CollectiveCall,32> &collectives) {
  for (const auto &cc : collectives) {
    if (!cc.insideConditional) continue;
    diag_.warning("CollectiveOrderRule",
        llvm::formatv("Collective '{0}' on communicator '{1}' is inside a "
          "conditional — not all ranks may reach it. Deadlock risk.",
          toString(cc.kind), cc.communicator).str(),
        cc.location,
        "Move collective outside the conditional or use MPI_Comm_split.");
  }
}

void CollectiveOrderRule::checkOrderConsistency(
    const llvm::SmallVector<CollectiveCall,32> &collectives) {
  std::unordered_map<std::string,std::vector<const CollectiveCall*>> byComm;
  for (const auto &cc : collectives)
    byComm[cc.communicator].push_back(&cc);
  for (auto &[comm,calls] : byComm) {
    std::sort(calls.begin(),calls.end(),
      [](const CollectiveCall*a,const CollectiveCall*b){
        return a->seqInScope<b->seqInScope;});
    for (size_t i=0;i+1<calls.size();++i) {
      if (calls[i]->seqInScope==calls[i+1]->seqInScope &&
          calls[i]->kind!=calls[i+1]->kind) {
        diag_.error("CollectiveOrderRule",
            llvm::formatv("Collective ordering conflict on comm '{0}': "
              "position {1} is '{2}' vs '{3}'. All ranks must call in same order.",
              comm, calls[i]->seqInScope,
              toString(calls[i]->kind),
              toString(calls[i+1]->kind)).str(),
            calls[i+1]->location);
      }
    }
  }
}

// ── RuleRunner ──────────────────────────────────────────────────────────────

RuleRunner::RuleRunner(DiagnosticEngine &diag) : diag_(diag) {
  rules_.push_back(std::make_unique<BufferSizeRule>(diag));
  rules_.push_back(std::make_unique<ContiguityRule>(diag));
  rules_.push_back(std::make_unique<DerivedTypeRule>(diag));
  rules_.push_back(std::make_unique<OptionalArgRule>(diag));
  rules_.push_back(std::make_unique<CollectiveOrderRule>(diag));
}

void RuleRunner::runPerCall(const llvm::SmallVector<MPICallMetadata,64> &calls) {
  for (const auto &call : calls)
    for (auto &rule : rules_)
      rule->check(call);
}

void RuleRunner::runEndOfTU(
    const llvm::SmallVector<MPICallMetadata,64>  &calls,
    const llvm::SmallVector<CollectiveCall,32>    &collectives) {
  for (auto &rule : rules_)
    rule->checkAll(calls, collectives);
}

} // namespace flang_mpi_checker
