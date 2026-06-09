#pragma once
#include "MPI/MPIMetadata.h"
#include "flang/Evaluate/call.h"
#include "flang/Evaluate/fold.h"
#include "flang/Evaluate/tools.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Semantics/semantics.h"
#include "llvm/ADT/SmallVector.h"
#include <list>
#include <optional>
#include <string>
#include <unordered_map>

namespace flang_mpi_checker {

int64_t intrinsicTypeBytes(llvm::StringRef typeName);
std::optional<int64_t> mpiDatatypeBytes(llvm::StringRef mpiTypeName);

class ArrayContiguityAnalyser {
public:
  static ContiguityKind analyse(
      const std::list<Fortran::parser::SectionSubscript> &subscripts,
      const Fortran::semantics::Symbol *sym);
  static ContiguityKind fromSymbol(const Fortran::semantics::Symbol *sym);
private:
  static ContiguityKind merge(ContiguityKind a, ContiguityKind b);
};

class MPICallExtractor {
public:
  explicit MPICallExtractor(Fortran::semantics::SemanticsContext &ctx);
  void extract(const Fortran::parser::Program &program);
  const llvm::SmallVector<MPICallMetadata, 64> &calls() const { return calls_; }
  const llvm::SmallVector<CollectiveCall, 32>  &collectiveCalls() const { return collectiveCalls_; }

  // exposed for visitor
  int conditionalDepth_{0};
  int loopDepth_{0};
  void visitCallStatement(const Fortran::parser::CallStmt &call);

private:
  // ── Semantic-level extraction (uses typedCall) ───────────────────────────
  ArrayDescriptor extractBufferFromSemanticArg(
      const Fortran::evaluate::ActualArgument &arg);
  ArrayDescriptor descriptorFromSymbol(
      const Fortran::semantics::Symbol *sym);
  std::string resolveDatatype(int64_t stubValue);

  // ── Parser-level fallback ────────────────────────────────────────────────
  std::optional<ArrayDescriptor> extractBufferDescriptor(
      const Fortran::parser::ActualArg &arg,
      const Fortran::semantics::Symbol *sym);
  std::optional<int64_t> extractIntegerArg(
      const Fortran::parser::ActualArg &arg);
  std::string extractDatatypeName(
      const Fortran::parser::ActualArg &arg);
  std::string extractCommunicatorName(
      const Fortran::parser::ActualArg &arg);
  const Fortran::semantics::Symbol *getSymbolFromArg(
      const Fortran::parser::ActualArg &arg);
  std::optional<int64_t> getIntLiteralFromArg(
      const Fortran::parser::ActualArg &arg);
  std::optional<int64_t> getParamValueFromSymbol(
      const Fortran::semantics::Symbol *sym);
  std::optional<DerivedTypeLayout> getDerivedTypeLayout(
      const Fortran::semantics::Symbol *sym);

  bool isInsideConditional() const { return conditionalDepth_ > 0; }
  bool isInsideLoop()        const { return loopDepth_ > 0; }

  Fortran::semantics::SemanticsContext      &ctx_;
  llvm::SmallVector<MPICallMetadata, 64>     calls_;
  llvm::SmallVector<CollectiveCall, 32>       collectiveCalls_;
  int collectiveSeqCounter_{0};
  std::string currentProcedure_{"<unknown>"};

  struct MPIArgMap { int bufferPos, countPos, datatypePos, destPos, tagPos, commPos; };
  std::unordered_map<std::string, MPIArgMap> argMap_;
  static std::unordered_map<std::string, MPIArgMap> buildArgMap();
};

} // namespace flang_mpi_checker
