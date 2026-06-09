#pragma once
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace flang_mpi_checker {

enum class Severity { Note, Warning, Error };
llvm::StringRef toString(Severity s);

struct Diagnostic {
  Severity    severity;
  std::string ruleName;
  std::string message;
  llvm::SMLoc loc;
  std::string suggestedFix;
};

class DiagnosticEngine {
public:
  explicit DiagnosticEngine(llvm::SourceMgr &srcMgr,
                             llvm::raw_ostream &os = llvm::errs());
  void note   (llvm::StringRef rule, llvm::StringRef msg, llvm::SMLoc loc);
  void warning(llvm::StringRef rule, llvm::StringRef msg, llvm::SMLoc loc,
               llvm::StringRef fix = {});
  void error  (llvm::StringRef rule, llvm::StringRef msg, llvm::SMLoc loc,
               llvm::StringRef fix = {});

  size_t warningCount() const { return warningCount_; }
  size_t errorCount()   const { return errorCount_; }
  size_t totalCount()   const { return diagnostics_.size(); }

  void printSummary(llvm::raw_ostream &os) const;
  void printJSON(llvm::raw_ostream &os) const;
  void printSARIF(llvm::raw_ostream &os) const;
  const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }

private:
  void emit(Severity sev, llvm::StringRef rule, llvm::StringRef msg,
            llvm::SMLoc loc, llvm::StringRef fix = {});
  std::string locationStr(llvm::SMLoc loc) const;

  llvm::SourceMgr        &srcMgr_;
  llvm::raw_ostream      &os_;
  std::vector<Diagnostic>  diagnostics_;
  size_t warningCount_{0}, errorCount_{0};
};

} // namespace flang_mpi_checker
