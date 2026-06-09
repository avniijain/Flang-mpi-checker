#pragma once
#include "Diagnostics/DiagnosticEngine.h"
#include "MPI/MPIMetadata.h"
#include "llvm/ADT/SmallVector.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace flang_mpi_checker {

class CorrectnessRule {
public:
  explicit CorrectnessRule(DiagnosticEngine &diag) : diag_(diag) {}
  virtual ~CorrectnessRule() = default;
  virtual void check(const MPICallMetadata &) {}
  virtual void checkAll(const llvm::SmallVector<MPICallMetadata, 64> &,
                        const llvm::SmallVector<CollectiveCall, 32>   &) {}
  virtual llvm::StringRef ruleName() const = 0;
protected:
  DiagnosticEngine &diag_;
};

class BufferSizeRule : public CorrectnessRule {
public:
  using CorrectnessRule::CorrectnessRule;
  void check(const MPICallMetadata &call) override;
  void checkAll(const llvm::SmallVector<MPICallMetadata, 64> &calls,
                const llvm::SmallVector<CollectiveCall, 32>  &) override;
  llvm::StringRef ruleName() const override { return "BufferSizeRule"; }
private:
  void checkBufferEnvelope(const MPICallMetadata &call);
  void matchSendRecvPairs(const llvm::SmallVector<MPICallMetadata, 64> &calls);
  struct PendingSend { int64_t envelopeBytes; llvm::SMLoc loc; };
  std::unordered_map<std::string, PendingSend> pendingSends_;
};

class ContiguityRule : public CorrectnessRule {
public:
  using CorrectnessRule::CorrectnessRule;
  void check(const MPICallMetadata &call) override;
  llvm::StringRef ruleName() const override { return "ContiguityRule"; }
private:
  void warnIfNonContiguous(const MPICallMetadata &call);
  void warnAssumedShape(const MPICallMetadata &call);
};

class DerivedTypeRule : public CorrectnessRule {
public:
  using CorrectnessRule::CorrectnessRule;
  void check(const MPICallMetadata &call) override;
  llvm::StringRef ruleName() const override { return "DerivedTypeRule"; }
private:
  void checkBindCOrSequence(const MPICallMetadata &call);
  void checkNoAllocatablePointer(const MPICallMetadata &call);
  void checkStructOffsets(const MPICallMetadata &call);
};

class OptionalArgRule : public CorrectnessRule {
public:
  using CorrectnessRule::CorrectnessRule;
  void check(const MPICallMetadata &call) override;
  llvm::StringRef ruleName() const override { return "OptionalArgRule"; }
private:
  void checkOptionalWithoutPresent(const MPICallMetadata &call);
  void checkStatusOptional(const MPICallMetadata &call);
};

class CollectiveOrderRule : public CorrectnessRule {
public:
  using CorrectnessRule::CorrectnessRule;
  void checkAll(const llvm::SmallVector<MPICallMetadata, 64>  &,
                const llvm::SmallVector<CollectiveCall, 32>    &collectives) override;
  llvm::StringRef ruleName() const override { return "CollectiveOrderRule"; }
private:
  void checkConditionalCollective(const llvm::SmallVector<CollectiveCall, 32> &c);
  void checkOrderConsistency(const llvm::SmallVector<CollectiveCall, 32> &c);
};

class RuleRunner {
public:
  explicit RuleRunner(DiagnosticEngine &diag);
  void runPerCall(const llvm::SmallVector<MPICallMetadata, 64> &calls);
  void runEndOfTU(const llvm::SmallVector<MPICallMetadata, 64>  &calls,
                  const llvm::SmallVector<CollectiveCall, 32>    &collectives);
  void addRule(std::unique_ptr<CorrectnessRule> rule) {
    rules_.push_back(std::move(rule));
  }
private:
  DiagnosticEngine &diag_;
  std::vector<std::unique_ptr<CorrectnessRule>> rules_;
};

} // namespace flang_mpi_checker
