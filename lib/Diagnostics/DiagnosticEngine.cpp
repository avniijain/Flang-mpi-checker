#include "Diagnostics/DiagnosticEngine.h"
#include "llvm/Support/FormatVariadic.h"
#include <unordered_map>

namespace flang_mpi_checker {

llvm::StringRef toString(Severity s) {
  switch(s) {
  case Severity::Note:    return "note";
  case Severity::Warning: return "warning";
  case Severity::Error:   return "error";
  }
  return "unknown";
}

DiagnosticEngine::DiagnosticEngine(llvm::SourceMgr &srcMgr, llvm::raw_ostream &os)
    : srcMgr_(srcMgr), os_(os) {}

std::string DiagnosticEngine::locationStr(llvm::SMLoc loc) const {
  if (!loc.isValid()) return "<unknown>";
  auto lc = srcMgr_.getLineAndColumn(loc);
  unsigned buf = srcMgr_.FindBufferContainingLoc(loc);
  auto name = srcMgr_.getMemoryBuffer(buf)->getBufferIdentifier();
  return llvm::formatv("{0}:{1}:{2}", name, lc.first, lc.second).str();
}

void DiagnosticEngine::emit(Severity sev, llvm::StringRef rule,
                             llvm::StringRef msg, llvm::SMLoc loc,
                             llvm::StringRef fix) {
  Diagnostic d;
  d.severity     = sev;
  d.ruleName     = rule.str();
  d.message      = msg.str();
  d.loc          = loc;
  d.suggestedFix = fix.str();
  diagnostics_.push_back(d);
  if (sev==Severity::Warning) ++warningCount_;
  if (sev==Severity::Error)   ++errorCount_;
  os_ << locationStr(loc) << ": " << toString(sev)
      << " [" << rule << "]: " << msg << "\n";
  if (!fix.empty()) os_ << "  fix: " << fix << "\n";
}

void DiagnosticEngine::note(llvm::StringRef rule, llvm::StringRef msg, llvm::SMLoc loc) {
  emit(Severity::Note, rule, msg, loc);
}
void DiagnosticEngine::warning(llvm::StringRef rule, llvm::StringRef msg,
                                llvm::SMLoc loc, llvm::StringRef fix) {
  emit(Severity::Warning, rule, msg, loc, fix);
}
void DiagnosticEngine::error(llvm::StringRef rule, llvm::StringRef msg,
                              llvm::SMLoc loc, llvm::StringRef fix) {
  emit(Severity::Error, rule, msg, loc, fix);
}

void DiagnosticEngine::printSummary(llvm::raw_ostream &os) const {
  os << "\n=== Flang MPI Checker Summary ===\n";
  os << "  Errors   : " << errorCount_   << "\n";
  os << "  Warnings : " << warningCount_ << "\n";
  os << "  Total    : " << diagnostics_.size() << "\n";
  std::unordered_map<std::string,int> byRule;
  for (const auto &d : diagnostics_) byRule[d.ruleName]++;
  os << "\n  By rule:\n";
  for (const auto &[rule,count] : byRule)
    os << "    " << rule << ": " << count << "\n";
}

void DiagnosticEngine::printJSON(llvm::raw_ostream &os) const {
  os << "[\n";
  for (size_t i=0; i<diagnostics_.size(); ++i) {
    const auto &d = diagnostics_[i];
    std::string msg;
    for (char c : d.message) {
      if (c=='"') msg+="\\\"";
      else if (c=='\\') msg+="\\\\";
      else if (c=='\n') msg+="\\n";
      else msg+=c;
    }
    os << "  {\"severity\":\"" << toString(d.severity)
       << "\",\"rule\":\"" << d.ruleName
       << "\",\"message\":\"" << msg << "\"}";
    if (i+1<diagnostics_.size()) os << ",";
    os << "\n";
  }
  os << "]\n";
}

void DiagnosticEngine::printSARIF(llvm::raw_ostream &os) const {
  os << "{\n  \"version\":\"2.1.0\",\n  \"runs\":[{\"tool\":{\"driver\":"
        "{\"name\":\"flang-mpi-checker\"}},\"results\":[\n";
  for (size_t i=0; i<diagnostics_.size(); ++i) {
    const auto &d = diagnostics_[i];
    std::string lv = (d.severity==Severity::Error)?"error":"warning";
    os << "    {\"ruleId\":\"" << d.ruleName
       << "\",\"level\":\"" << lv
       << "\",\"message\":{\"text\":\"" << d.message << "\"}}";
    if (i+1<diagnostics_.size()) os << ",";
    os << "\n";
  }
  os << "  ]}]}\n";
}

} // namespace flang_mpi_checker
