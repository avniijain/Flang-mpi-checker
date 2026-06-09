add_library(FlangMPICheckerAnalysis STATIC
  CorrectnessRules.cpp
)
target_include_directories(FlangMPICheckerAnalysis PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(FlangMPICheckerAnalysis PUBLIC
  FlangMPICheckerMPI
  FlangMPICheckerDiagnostics
  LLVMSupport
)
