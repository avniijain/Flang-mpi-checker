add_library(FlangMPICheckerDiagnostics STATIC
  DiagnosticEngine.cpp
)
target_include_directories(FlangMPICheckerDiagnostics PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(FlangMPICheckerDiagnostics PUBLIC
  LLVMSupport
)
