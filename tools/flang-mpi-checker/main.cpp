#include "Analysis/CorrectnessRules.h"
#include "Diagnostics/DiagnosticEngine.h"
#include "MPI/MPICallExtractor.h"
#include "MPI/MPIMetadata.h"

#include "flang/Frontend/CompilerInstance.h"
#include "flang/Frontend/CompilerInvocation.h"
#include "flang/Frontend/FrontendActions.h"
#include "flang/Parser/parsing.h"
#include "flang/Parser/provenance.h"
#include "flang/Semantics/semantics.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace flang_mpi_checker;

static cl::OptionCategory CheckerCat("Flang MPI Checker Options");

static cl::list<std::string> InputFiles(
    cl::Positional, cl::desc("<Fortran source files>"),
    cl::OneOrMore, cl::cat(CheckerCat));

static cl::list<std::string> ModuleDirs(
    "I", cl::desc("Module search directory (for .mod files)"),
    cl::cat(CheckerCat));

static cl::opt<std::string> OutputFormat(
    "output-format",
    cl::desc("Output format: text (default), json, sarif"),
    cl::init("text"), cl::cat(CheckerCat));

static cl::opt<std::string> OutputFile(
    "o", cl::desc("Write output to file (default: stderr)"),
    cl::init("-"), cl::cat(CheckerCat));

static cl::opt<bool> NoErrorOnWarning(
    "no-error-on-warning",
    cl::desc("Exit 0 even when warnings are emitted"),
    cl::init(false), cl::cat(CheckerCat));

static cl::opt<unsigned> MaxErrors(
    "max-errors",
    cl::desc("Stop after N errors (default 100)"),
    cl::init(100), cl::cat(CheckerCat));

static cl::opt<bool> Verbose(
    "v", cl::desc("Verbose: print extracted MPI call metadata"),
    cl::init(false), cl::cat(CheckerCat));

static int analyseFile(const std::string &filename,
                        DiagnosticEngine  &diagEngine) {
  Fortran::parser::AllSources allSources;
  Fortran::parser::AllCookedSources allCooked(allSources);
  Fortran::parser::Parsing parsing(allCooked);

  Fortran::parser::Options parseOptions;
  parseOptions.features.Enable(Fortran::common::LanguageFeature::OpenMP);

  // Add module search directories (-I flags)
  for (const auto &dir : ModuleDirs)
    parseOptions.searchDirectories.push_back(dir);

  parsing.Prescan(filename, parseOptions);
  if (parsing.messages().AnyFatalError()) {
    llvm::errs() << "error: prescan errors in '" << filename << "'\n";
    parsing.messages().Emit(llvm::errs(), allCooked);
    return 1;
  }

  parsing.Parse(llvm::outs());
  if (parsing.messages().AnyFatalError()) {
    llvm::errs() << "error: parse errors in '" << filename << "'\n";
    parsing.messages().Emit(llvm::errs(), allCooked);
    return 1;
  }

  auto &parseTreeOpt = parsing.parseTree();
  if (!parseTreeOpt) {
    llvm::errs() << "error: no parse tree for '" << filename << "'\n";
    return 1;
  }

  Fortran::semantics::SemanticsContext semCtx(
      Fortran::common::IntrinsicTypeDefaultKinds{},
      parseOptions.features,
      allCooked);

  // Set module directory search paths for semantics
  for (const auto &dir : ModuleDirs)
    semCtx.set_moduleDirectory(dir);

  Fortran::semantics::Semantics semantics{semCtx, *parseTreeOpt};
  semantics.Perform();
  if (semCtx.AnyFatalError()) {
    if (Verbose) {
      llvm::errs() << "note: semantic errors in '" << filename
                   << "' (checker still applied)\n";
    }
    // Don't return — still run the checker on what we have
  }

  MPICallExtractor extractor{semCtx};
  extractor.extract(*parseTreeOpt);

  if (Verbose) {
    llvm::outs() << "\n[flang-mpi-checker] " << filename << "\n";
    llvm::outs() << "  MPI calls   : " << extractor.calls().size() << "\n";
    llvm::outs() << "  Collectives : " << extractor.collectiveCalls().size() << "\n";
    for (const auto &c : extractor.calls())
      llvm::outs() << "    " << toString(c.kind)
                   << " @ " << c.calleeName << "\n";
  }

  RuleRunner runner{diagEngine};
  runner.runPerCall(extractor.calls());
  runner.runEndOfTU(extractor.calls(), extractor.collectiveCalls());
  return 0;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);
  cl::HideUnrelatedOptions(CheckerCat);
  cl::ParseCommandLineOptions(argc, argv,
      "flang-mpi-checker - Static MPI correctness analysis for Fortran\n");

  std::error_code ec;
  llvm::ToolOutputFile outFile(OutputFile, ec, llvm::sys::fs::OF_Text);
  if (ec) {
    llvm::errs() << "error: cannot open output: " << ec.message() << "\n";
    return 1;
  }

  llvm::SourceMgr globalSrcMgr;
  DiagnosticEngine diagEngine{globalSrcMgr, llvm::errs()};

  int overallRc = 0;
  for (const auto &file : InputFiles) {
    int rc = analyseFile(file, diagEngine);
    if (rc != 0) overallRc = rc;
    if (diagEngine.errorCount() >= MaxErrors) {
      llvm::errs() << "note: stopped after " << MaxErrors << " errors.\n";
      break;
    }
  }

  if (OutputFormat == "json")
    diagEngine.printJSON(outFile.os());
  else if (OutputFormat == "sarif")
    diagEngine.printSARIF(outFile.os());
  else
    diagEngine.printSummary(outFile.os());

  outFile.keep();

  if (overallRc != 0) return overallRc;
  if (diagEngine.errorCount() > 0) return 2;
  if (!NoErrorOnWarning && diagEngine.warningCount() > 0) return 1;
  return 0;
}