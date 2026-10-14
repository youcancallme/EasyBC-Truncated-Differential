cmake_minimum_required(VERSION 3.16)
project(EasyBC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 可配置的变量 (Configurable Paths) ---
set(GUROBI_DIR "$ENV{GUROBI_HOME}" CACHE PATH "Path to Gurobi installation")
set(GUROBI_LIBRARY_NAME "gurobi130" CACHE STRING "Gurobi C library name (match installed version)")
set(Z3_DIR "$ENV{Z3_DIR}" CACHE PATH "Z3 distribution with include/ and bin/ or lib/")
set(PBLIB_DIR "$ENV{PBLIB_DIR}" CACHE PATH "PBLib source tree")
if(NOT Z3_DIR OR NOT PBLIB_DIR)
    message(FATAL_ERROR "Set Z3_DIR and PBLIB_DIR to locally installed dependencies")
endif()

find_path(GUROBI_INCLUDE_DIR gurobi_c++.h HINTS "${GUROBI_DIR}/include")
find_path(Z3_INCLUDE_DIR z3++.h PATHS "${Z3_DIR}/include" NO_DEFAULT_PATH)
find_library(Z3_LIBRARY NAMES z3 PATHS "${Z3_DIR}/bin" "${Z3_DIR}/lib" "${Z3_DIR}/lib64" NO_DEFAULT_PATH)
foreach(dependency GUROBI_INCLUDE_DIR Z3_INCLUDE_DIR Z3_LIBRARY)
    if(NOT ${dependency})
        message(FATAL_ERROR "Missing ${dependency}; configure GUROBI_DIR and Z3_DIR for your installation")
    endif()
endforeach()
if(NOT EXISTS "${PBLIB_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "PBLIB_DIR must point to a PBLib source tree")
endif()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/preprocessing)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/branchNum)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/sbox)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/util)
INCLUDE_DIRECTORIES(${Z3_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/truncatedDifferentialBPT)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/branchingPropertyTable)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib/jsoncpp)

# 包含 Gurobi 的头文件
include_directories(${GUROBI_INCLUDE_DIR})
# 链接 Gurobi 的库文件
link_directories(${GUROBI_DIR}/lib)

find_package(OpenMP REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -gdwarf-2")

set(GUROBI_LIB "${GUROBI_DIR}/lib")  # 假设库文件在 'lib' 子目录下

FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)
message( ${CMAKE_CURRENT_SOURCE_DIR} )
BISON_TARGET(parser lib/language/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
        )
FLEX_TARGET(lexer lib/language/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)

ADD_FLEX_BISON_DEPENDENCY(lexer parser)

ADD_EXECUTABLE(EasyBC
        ${FLEX_lexer_OUTPUTS}
        ${BISON_parser_OUTPUTS}
        main.cpp
        include/preprocessing/Value.h
        include/preprocessing/Interpreter.h
        include/preprocessing/ValueCommon.h
        include/preprocessing/ThreeAddressNode.h
        include/preprocessing/ProcedureH.h
        include/preprocessing/Transformer.h
        include/branchNum/BranchNum.h
        include/differential/DiffSBMILP.h
        include/differential/DiffSWMILP.h
        include/differential/DiffMILPcons.h
        include/interpreter/semanticInter.h
        include/sbox/SboxModel.h
        include/sbox/SboxExtC.h
        include/sbox/Check.h
        include/sbox/Reduction.h
        include/sbox/superball.h
        include/sbox/superballMGR.h
        include/sbox/SyntaxGuided.h
        include/util/utilities.h
        lib/preprocessing/ASTNode.cpp
        lib/preprocessing/ValueCommon.cpp
        lib/preprocessing/ProcedureH.cpp
        lib/preprocessing/Transformer.cpp
        lib/branchNum/BranchNum.cpp
        lib/interpreter/semanticInter.cpp
        lib/sbox/SboxModel.cpp
        lib/sbox/Check.cpp
        lib/sbox/Reduction.cpp
        lib/sbox/superball.cpp
        lib/sbox/superballMGR.cpp
        lib/util/utilities.cpp
        lib/differential/DiffSBMILP.cpp
        lib/differential/DiffSWMILP.cpp
        lib/differential/DiffMILPcons.cpp
        include/truncatedDifferentialBPT/TruncDiffBPTMILPcons.h
        include/truncatedDifferentialBPT/TruncDiffBPTSWMILP.h
        lib/truncatedDifferentialBPT/TruncDiffBPTMILPcons.cpp
        lib/truncatedDifferentialBPT/TruncDiffBPTSWMILP.cpp
        include/branchingPropertyTable/BranchingPropertyTableCalculator.h
        include/branchingPropertyTable/BPTProbExtC.h
        lib/branchingPropertyTable/BranchingPropertyTableCalculator.cpp
        lib/branchingPropertyTable/BPTReduction.cpp
        include/branchingPropertyTable/BPTReduction.h
        lib/branchingPropertyTable/BPTCheck.cpp
        include/branchingPropertyTable/BPTCheck.h
        include/truncatedDifferentialBPT/TruncDiffBPTSATcons.h
        include/truncatedDifferentialBPT/TruncDiffBPTSWSAT.h
        lib/truncatedDifferentialBPT/TruncDiffBPTSATcons.cpp
        lib/truncatedDifferentialBPT/TruncDiffBPTSWSAT.cpp
        lib/branchingPropertyTable/BranchingPropertyTableCalculatorLA.cpp
        lib/jsoncpp/jsoncpp.cpp
)

# 查找静态库
find_library(gurobi_static NAMES gurobi_c++  # 注意这里不需要前缀 'lib' 和后缀 '.a'
        PATHS ${GUROBI_LIB})

# 查找动态库
find_library(gurobi_dynamic NAMES ${GUROBI_LIBRARY_NAME}
        PATHS ${GUROBI_LIB})

if(NOT gurobi_static OR NOT gurobi_dynamic)
    message(FATAL_ERROR "Missing Gurobi C++ or C library; set GUROBI_DIR and GUROBI_LIBRARY_NAME")
endif()
message(${gurobi_static})
message(${gurobi_dynamic})

# PBLib uses its own language standard; EasyBC requires C++17.
add_subdirectory("${PBLIB_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pblib")

target_include_directories(EasyBC PRIVATE
        ${PBLIB_DIR}
        ${PBLIB_DIR}/pblib
)

TARGET_LINK_LIBRARIES(EasyBC ${FLEX_LIBRARIES} OpenMP::OpenMP_CXX)
TARGET_LINK_LIBRARIES(EasyBC ${gurobi_static} ${gurobi_dynamic} ${Z3_LIBRARY})
TARGET_LINK_LIBRARIES(EasyBC pblib)
