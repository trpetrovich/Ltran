cmake_minimum_required(VERSION 3.20)

project(Ltran
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ltran
    src/main.cpp
)

target_include_directories(ltran PRIVATE
    include
)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

target_include_directories(ltran PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(ltran PRIVATE
    ${LLVM_DEFINITIONS}
)

target_link_libraries(ltran PRIVATE
    LLVMCore
    LLVMSupport
    LLVMIRReader
)
