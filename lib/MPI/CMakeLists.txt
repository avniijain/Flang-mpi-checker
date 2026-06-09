add_library(FlangMPICheckerMPI STATIC
  MPIMetadata.cpp
  MPICallExtractor.cpp
)
target_include_directories(FlangMPICheckerMPI PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(FlangMPICheckerMPI PUBLIC
  LLVMSupport
  FortranParser
  FortranSemantics
  FortranEvaluate
  FortranCommon
)
