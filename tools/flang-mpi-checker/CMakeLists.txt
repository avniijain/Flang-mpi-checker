add_executable(flang-mpi-checker main.cpp)

target_include_directories(flang-mpi-checker PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

target_link_libraries(flang-mpi-checker PRIVATE
  FlangMPICheckerAnalysis
  FlangMPICheckerMPI
  FlangMPICheckerDiagnostics
  # Flang libraries (order matters for static linking)
  FortranSemantics
  FortranEvaluate
  FortranParser
  FortranDecimal
  FortranCommon
  # LLVM libraries needed by Flang semantics
  LLVMFrontendOpenMP
  LLVMFrontendOpenACC
  LLVMTargetParser
  LLVMSupport
  LLVMOption
  LLVMDemangle
)

install(TARGETS flang-mpi-checker RUNTIME DESTINATION bin)
