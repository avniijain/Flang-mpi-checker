#include "MPI/MPICallExtractor.h"
#include "flang/Evaluate/call.h"
#include "flang/Evaluate/fold.h"
#include "flang/Evaluate/tools.h"
#include "flang/Parser/parse-tree-visitor.h"
#include "flang/Semantics/tools.h"
#include "llvm/ADT/StringSwitch.h"
#include <algorithm>
#include <cctype>
#include <list>
#include <vector>

namespace flang_mpi_checker {

static std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return std::tolower(c); });
  return s;
}

// ── Datatype size tables ────────────────────────────────────────────────────

int64_t intrinsicTypeBytes(llvm::StringRef t) {
  return llvm::StringSwitch<int64_t>(t.lower())
    .Case("integer",4).Case("real",4).Case("double precision",8)
    .Case("complex",8).Case("logical",4).Case("character",1).Default(0);
}

std::optional<int64_t> mpiDatatypeBytes(llvm::StringRef name) {
  std::string s = name.lower();
  if (s.rfind("mpi_",0)==0) s = s.substr(4);
  int64_t sz = llvm::StringSwitch<int64_t>(s)
    .Case("integer",4).Case("integer1",1).Case("integer2",2)
    .Case("integer4",4).Case("integer8",8)
    .Case("real",4).Case("real4",4).Case("real8",8)
    .Case("double_precision",8).Case("complex",8)
    .Case("complex8",8).Case("complex16",16).Case("double_complex",16)
    .Case("logical",4).Case("character",1).Case("byte",1)
    .Default(-1);
  if (sz < 0) return std::nullopt;
  return sz;
}

// ── Contiguity analyser ─────────────────────────────────────────────────────

ContiguityKind ArrayContiguityAnalyser::merge(ContiguityKind a, ContiguityKind b) {
  if (a==ContiguityKind::NonContiguous||b==ContiguityKind::NonContiguous)
    return ContiguityKind::NonContiguous;
  if (a==ContiguityKind::MaybeContiguous||b==ContiguityKind::MaybeContiguous)
    return ContiguityKind::MaybeContiguous;
  if (a==ContiguityKind::Unknown||b==ContiguityKind::Unknown)
    return ContiguityKind::Unknown;
  return ContiguityKind::Contiguous;
}

ContiguityKind ArrayContiguityAnalyser::fromSymbol(
    const Fortran::semantics::Symbol *sym) {
  if (!sym) return ContiguityKind::Unknown;
  const auto &ult = sym->GetUltimate();
  if (ult.attrs().test(Fortran::semantics::Attr::CONTIGUOUS))
    return ContiguityKind::Contiguous;
  if (const auto *ao =
        ult.detailsIf<Fortran::semantics::ObjectEntityDetails>()) {
    if (ao->IsAssumedShape()) return ContiguityKind::MaybeContiguous;
    const auto &shape = ao->shape();
    if (!shape.empty() && shape.back().ubound().isStar())
      return ContiguityKind::Contiguous;
  }
  if (ult.attrs().test(Fortran::semantics::Attr::POINTER))
    return ContiguityKind::MaybeContiguous;
  return ContiguityKind::Contiguous;
}

ContiguityKind ArrayContiguityAnalyser::analyse(
    const std::list<Fortran::parser::SectionSubscript> &subscripts,
    const Fortran::semantics::Symbol *sym) {
  if (subscripts.empty()) return fromSymbol(sym);
  ContiguityKind result = ContiguityKind::Contiguous;
  bool sawScalarSubscript = false;
  for (const auto &sub : subscripts) {
    Fortran::common::visit(Fortran::common::visitors{
      [&](const Fortran::parser::IntExpr &) {
        sawScalarSubscript = true;
      },
      [&](const Fortran::parser::SubscriptTriplet &triplet) {
        if (sawScalarSubscript)
          result = merge(result, ContiguityKind::NonContiguous);
        const auto &stride = std::get<2>(triplet.t);
        ContiguityKind dimKind = stride
          ? ContiguityKind::MaybeContiguous
          : ContiguityKind::Contiguous;
        result = merge(result, dimKind);
      },
    }, sub.u);
  }
  return result;
}

// ── Arg-position table ──────────────────────────────────────────────────────

std::unordered_map<std::string, MPICallExtractor::MPIArgMap>
MPICallExtractor::buildArgMap() {
  std::unordered_map<std::string, MPIArgMap> m;
  m["mpi_send"]      = {0,1,2,3,4,5};
  m["mpi_recv"]      = {0,1,2,3,4,5};
  m["mpi_isend"]     = {0,1,2,3,4,5};
  m["mpi_irecv"]     = {0,1,2,3,4,5};
  m["mpi_bcast"]     = {0,1,2,-1,-1,4};
  m["mpi_reduce"]    = {0,-1,2,3,-1,-1};
  m["mpi_allreduce"] = {0,-1,2,3,-1,-1};
  m["mpi_gather"]    = {0,1,2,-1,-1,-1};
  m["mpi_scatter"]   = {0,1,2,-1,-1,-1};
  m["mpi_allgather"] = {0,1,2,-1,-1,-1};
  m["mpi_alltoall"]  = {0,1,2,-1,-1,-1};
  return m;
}

MPICallExtractor::MPICallExtractor(Fortran::semantics::SemanticsContext &ctx)
    : ctx_(ctx), argMap_(buildArgMap()) {}

// ── Visitor ─────────────────────────────────────────────────────────────────

namespace {
struct CallCollector {
  MPICallExtractor &ex;
  template<typename T> bool Pre(const T &) { return true; }
  template<typename T> void Post(const T &) {}

  bool Pre(const Fortran::parser::CallStmt &call) {
    ex.visitCallStatement(call);
    return true;
  }
  bool Pre(const Fortran::parser::IfConstruct &) {
    return true;
  }
  void Post(const Fortran::parser::IfConstruct &) {
  }
  bool Pre(const Fortran::parser::IfThenStmt &) {
    ++ex.conditionalDepth_; return true;
  }
  void Post(const Fortran::parser::EndIfStmt &) {
    if (ex.conditionalDepth_>0) --ex.conditionalDepth_;
  }
  bool Pre(const Fortran::parser::IfStmt &) {
    ++ex.conditionalDepth_; return true;
  }
  void Post(const Fortran::parser::IfStmt &) {
    if (ex.conditionalDepth_>0) --ex.conditionalDepth_;
  }
  bool Pre(const Fortran::parser::DoConstruct &) {
    ++ex.loopDepth_; return true;
  }
  void Post(const Fortran::parser::DoConstruct &) {
    if (ex.loopDepth_>0) --ex.loopDepth_;
  }
};
} // anon

void MPICallExtractor::extract(const Fortran::parser::Program &program) {
  CallCollector visitor{*this};
  Fortran::parser::Walk(program, visitor);
}

// ── Call-site visitor ────────────────────────────────────────────────────────

void MPICallExtractor::visitCallStatement(
    const Fortran::parser::CallStmt &callStmt) {

  // Get callee name from parser
  const auto &pd = std::get<0>(callStmt.call.t);
  std::string calleeName;
  if (const auto *name = std::get_if<Fortran::parser::Name>(&pd.u))
    calleeName = toLower(name->ToString());
  else return;

  MPICallKind kind = mpiCallKindFromName(calleeName);
  if (kind == MPICallKind::Unknown) return;

  MPICallMetadata meta;
  meta.kind              = kind;
  meta.calleeName        = calleeName;
  meta.insideConditional = isInsideConditional();
  meta.insideLoop        = isInsideLoop();
  meta.nestingDepth      = conditionalDepth_ + loopDepth_;

  const auto &argList = std::get<1>(callStmt.call.t);
  std::vector<const Fortran::parser::ActualArg *> args;
  for (const auto &spec : argList)
    args.push_back(&std::get<Fortran::parser::ActualArg>(spec.t));

  auto it = argMap_.find(calleeName);
  if (it != argMap_.end()) {
    const MPIArgMap &am = it->second;
    auto valid = [&](int pos) { return pos >= 0 && pos < (int)args.size(); };

    if (valid(am.bufferPos)) {
      const auto *sym = getSymbolFromArg(*args[am.bufferPos]);
      meta.buffer = extractBufferDescriptor(*args[am.bufferPos], sym);
      meta.derivedTypeLayout = getDerivedTypeLayout(sym);
    }
    if (valid(am.countPos)) {
      meta.count = getIntLiteralFromArg(*args[am.countPos]);
      if (!meta.count)
        meta.count = getParamValueFromSymbol(getSymbolFromArg(*args[am.countPos]));
    }
    if (valid(am.datatypePos)) {
      meta.mpiDatatype = extractDatatypeName(*args[am.datatypePos]);
      auto dtValue = getParamValueFromSymbol(getSymbolFromArg(*args[am.datatypePos]));
      if (meta.mpiDatatype.empty() && dtValue)
        meta.mpiDatatype = resolveDatatype(*dtValue);
      meta.datatypeBytes = mpiDatatypeBytes(meta.mpiDatatype);
    }
    if (valid(am.tagPos)) {
      if (auto value = getIntLiteralFromArg(*args[am.tagPos]))
        meta.tag = static_cast<int>(*value);
    }
    if (valid(am.commPos))
      meta.communicator = extractCommunicatorName(*args[am.commPos]);

    for (const auto *arg : args) {
      const auto *sym = getSymbolFromArg(*arg);
      if (!sym || !sym->GetUltimate().attrs().test(Fortran::semantics::Attr::OPTIONAL))
        continue;
      OptionalArgInfo info;
      info.argName = toLower(sym->name().ToString());
      info.isDeclaredOptional = true;
      info.isPresentAtCallSite = true;
      info.mayBeAbsent = !isInsideConditional() ||
          info.argName.find("status") != std::string::npos;
      meta.optionalArgs.push_back(std::move(info));
    }
  }

  if (isCollective(kind)) {
    CollectiveCall cc;
    cc.kind              = kind;
    cc.communicator      = meta.communicator;
    cc.insideConditional = isInsideConditional();
    cc.seqInScope        = collectiveSeqCounter_++;
    cc.procedureName     = currentProcedure_;
    collectiveCalls_.push_back(cc);
    meta.collectiveSeq   = cc.seqInScope;
  }
  calls_.push_back(std::move(meta));
}

// ── Extract array descriptor from semantic ActualArgument ───────────────────

ArrayDescriptor MPICallExtractor::extractBufferFromSemanticArg(
    const Fortran::evaluate::ActualArgument &arg) {
  ArrayDescriptor desc;

  // Get the expression from the semantic argument
  const Fortran::evaluate::Expr<Fortran::evaluate::SomeType> *expr =
      arg.UnwrapExpr();
  if (!expr) return desc;

  // Try to get the underlying symbol (whole array reference)
  if (const Fortran::semantics::Symbol *sym =
        Fortran::evaluate::UnwrapWholeSymbolDataRef(*expr)) {
    desc = descriptorFromSymbol(sym);
  } else {
    // Array section — check for non-contiguous subscripts
    desc.isSection  = true;
    desc.contiguity = ContiguityKind::MaybeContiguous;

    // Walk the expression to find array element/section
    // If the expression has a non-unit stride we mark NonContiguous
    // For now mark as MaybeContiguous (conservative)
    desc.contiguity = ContiguityKind::MaybeContiguous;
  }

  return desc;
}

ArrayDescriptor MPICallExtractor::descriptorFromSymbol(
    const Fortran::semantics::Symbol *sym) {
  ArrayDescriptor desc;
  if (!sym) return desc;

  const auto &ult = sym->GetUltimate();
  desc.contiguity = ArrayContiguityAnalyser::fromSymbol(sym);
  desc.hasCONTIGUOUSAttr =
      ult.attrs().test(Fortran::semantics::Attr::CONTIGUOUS);

  if (const auto *ao =
        ult.detailsIf<Fortran::semantics::ObjectEntityDetails>()) {
    desc.isAssumedShape = ao->IsAssumedShape();
    desc.rank           = ao->shape().Rank();

    // Get element byte size from intrinsic type
    if (const auto *type = ult.GetType()) {
      if (const auto *intrinsic =
            type->AsIntrinsic()) {
        auto kindExpr = intrinsic->kind();
        auto kindOpt = Fortran::evaluate::ToInt64(kindExpr);
        if (kindOpt && *kindOpt > 0) desc.elementBytes = *kindOpt;
      }
    }

    // Get extent of each dimension
    for (const auto &shapeSpec : ao->shape()) {
      ArrayDescriptor::DimInfo dim;
      // For explicit shape arrays, try to evaluate bounds
      if (const auto &lower = shapeSpec.lbound().GetExplicit()) {
        dim.lower = Fortran::evaluate::ToInt64(*lower);
      }
      if (const auto &upper = shapeSpec.ubound().GetExplicit()) {
        dim.upper = Fortran::evaluate::ToInt64(*upper);
        if (dim.lower && dim.upper)
          dim.extent = *dim.upper - *dim.lower + 1;
      }
      desc.dims.push_back(dim);
    }
  }
  return desc;
}

// ── Resolve MPI datatype value to name ──────────────────────────────────────
// Our stub assigns: INTEGER=1, REAL=2, DOUBLE_PRECISION=3, etc.

std::string MPICallExtractor::resolveDatatype(int64_t val) {
  switch (val) {
  case 1:  return "mpi_integer";
  case 2:  return "mpi_real";
  case 3:  return "mpi_double_precision";
  case 4:  return "mpi_complex";
  case 5:  return "mpi_logical";
  case 6:  return "mpi_character";
  case 7:  return "mpi_byte";
  default: return "";
  }
}

// ── Parser-level fallback helpers ───────────────────────────────────────────

namespace {
struct SectionFinder {
  ArrayDescriptor &desc;
  const Fortran::semantics::Symbol *sym;

  template<typename T> bool Pre(const T &) { return true; }
  template<typename T> void Post(const T &) {}

  bool Pre(const Fortran::parser::ArrayElement &element) {
    bool hasTriplet = std::any_of(element.subscripts.begin(),
        element.subscripts.end(), [](const auto &subscript) {
          return std::holds_alternative<Fortran::parser::SubscriptTriplet>(
              subscript.u);
        });
    if (hasTriplet) {
      desc.isSection = true;
      desc.contiguity = ArrayContiguityAnalyser::analyse(element.subscripts, sym);
    }
    return true;
  }
};
} // namespace

std::optional<ArrayDescriptor>
MPICallExtractor::extractBufferDescriptor(
    const Fortran::parser::ActualArg &arg,
    const Fortran::semantics::Symbol *sym) {
  ArrayDescriptor desc;
  if (sym) {
    desc = descriptorFromSymbol(sym);
  }
  SectionFinder finder{desc, sym};
  Fortran::parser::Walk(arg, finder);
  return desc;
}

std::optional<int64_t>
MPICallExtractor::extractIntegerArg(const Fortran::parser::ActualArg &arg) {
  if (auto value = getIntLiteralFromArg(arg)) return value;
  return getParamValueFromSymbol(getSymbolFromArg(arg));
}

std::string
MPICallExtractor::extractDatatypeName(const Fortran::parser::ActualArg &arg) {
  if (const auto *ind =
        std::get_if<Fortran::common::Indirection<Fortran::parser::Expr>>(&arg.u)) {
    std::string result;
    Fortran::common::visit(Fortran::common::visitors{
      [&](const Fortran::common::Indirection<Fortran::parser::Designator> &desInd) {
        Fortran::common::visit(Fortran::common::visitors{
          [&](const Fortran::parser::DataRef &dr) {
            Fortran::common::visit(Fortran::common::visitors{
              [&](const Fortran::parser::Name &n) {
                result = toLower(n.ToString());
              },
              [](const auto &) {},
            }, dr.u);
          },
          [](const auto &) {},
        }, desInd.value().u);
      },
      [](const auto &) {},
    }, ind->value().u);
    if (!result.empty()) return result;
  }
  return "";
}

std::string
MPICallExtractor::extractCommunicatorName(const Fortran::parser::ActualArg &arg) {
  if (const auto *ind =
        std::get_if<Fortran::common::Indirection<Fortran::parser::Expr>>(&arg.u)) {
    std::string result;
    Fortran::common::visit(Fortran::common::visitors{
      [&](const Fortran::common::Indirection<Fortran::parser::Designator> &desInd) {
        Fortran::common::visit(Fortran::common::visitors{
          [&](const Fortran::parser::DataRef &dr) {
            Fortran::common::visit(Fortran::common::visitors{
              [&](const Fortran::parser::Name &n) {
                result = toLower(n.ToString());
              },
              [](const auto &) {},
            }, dr.u);
          },
          [](const auto &) {},
        }, desInd.value().u);
      },
      [](const auto &) {},
    }, ind->value().u);
    if (!result.empty()) return result;
  }
  return "mpi_comm_world";
}

namespace {
struct ArgFacts {
  const Fortran::semantics::Symbol *symbol{nullptr};
  std::optional<int64_t> literal;

  template<typename T> bool Pre(const T &) { return true; }
  template<typename T> void Post(const T &) {}

  bool Pre(const Fortran::parser::Name &name) {
    if (!symbol && name.symbol) symbol = name.symbol;
    return true;
  }
  bool Pre(const Fortran::parser::IntLiteralConstant &value) {
    if (!literal) {
      try {
        literal = std::stoll(std::get<0>(value.t).ToString());
      } catch (...) {
      }
    }
    return true;
  }
};
} // namespace

const Fortran::semantics::Symbol *MPICallExtractor::getSymbolFromArg(
    const Fortran::parser::ActualArg &arg) {
  ArgFacts facts;
  Fortran::parser::Walk(arg, facts);
  return facts.symbol;
}

std::optional<int64_t> MPICallExtractor::getIntLiteralFromArg(
    const Fortran::parser::ActualArg &arg) {
  ArgFacts facts;
  Fortran::parser::Walk(arg, facts);
  return facts.literal;
}

std::optional<int64_t> MPICallExtractor::getParamValueFromSymbol(
    const Fortran::semantics::Symbol *sym) {
  if (!sym) return std::nullopt;
  const auto &ultimate = sym->GetUltimate();
  if (const auto *object =
          ultimate.detailsIf<Fortran::semantics::ObjectEntityDetails>()) {
    if (object->init()) return Fortran::evaluate::ToInt64(*object->init());
  }
  return std::nullopt;
}

std::optional<DerivedTypeLayout> MPICallExtractor::getDerivedTypeLayout(
    const Fortran::semantics::Symbol *sym) {
  if (!sym) return std::nullopt;
  const auto &ultimate = sym->GetUltimate();
  const auto *type = ultimate.GetType();
  const auto *derived = type ? type->AsDerived() : nullptr;
  if (!derived) return std::nullopt;

  DerivedTypeLayout layout;
  const auto &typeSymbol = derived->typeSymbol().GetUltimate();
  layout.typeName = toLower(derived->name().ToString());
  layout.hasBindC = typeSymbol.attrs().test(Fortran::semantics::Attr::BIND_C);
  const auto *details =
      typeSymbol.detailsIf<Fortran::semantics::DerivedTypeDetails>();
  layout.hasSequence = details && details->sequence();

  const auto *scope = derived->GetScope();
  if (!details || !scope) return layout;
  for (const auto &componentName : details->componentNames()) {
    const auto *component = scope->FindComponent(componentName);
    if (!component) continue;
    DerivedTypeLayout::ComponentInfo info;
    info.name = toLower(componentName.ToString());
    const auto &componentUltimate = component->GetUltimate();
    info.isAllocatable =
        componentUltimate.attrs().test(Fortran::semantics::Attr::ALLOCATABLE);
    info.isPointer =
        componentUltimate.attrs().test(Fortran::semantics::Attr::POINTER);
    layout.hasAllocatableComponent |= info.isAllocatable;
    layout.hasPointerComponent |= info.isPointer;
    layout.components.push_back(std::move(info));
  }
  return layout;
}

} // namespace flang_mpi_checker
