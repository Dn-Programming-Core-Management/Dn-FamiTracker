# TODO: https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/

cmake_minimum_required(VERSION 3.10)

set(project "Dn_FamiTracker")
set(exe "Dn_FamiTracker")

project(${project})


# MFC based off https://github.com/Kitware/CMake/blob/master/Tests/MFC/CMakeLists.txt
    # also sets https://stackoverflow.com/questions/14172856/cmake-mt-md
# Simpler approach (unimplemented): https://stackoverflow.com/questions/11580748/cmake-mfc
    # does not set mt vs md

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    # Debug build requires dynamic MFC.
    set(CMAKE_MFC_FLAG_VALUE 0)
else ()
    # Otherwise link MFC statically.
    set(CMAKE_MFC_FLAG_VALUE 1)
endif ()

set(mfcin cmake/mfc.cmake.in)
set(mfc ${CMAKE_CURRENT_BINARY_DIR}/mfc.cmake)

# Configure MFC
configure_file(${mfcin} ${mfc} @ONLY)
include(${mfc})


# compiling
include(cmake/exe.cmake)
target_compile_features(${exe} PRIVATE cxx_std_17)

if(COMMAND target_precompile_headers)
    target_precompile_headers(${exe} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:Source/stdafx.cpp>"
    )
endif()


# linking
TARGET_LINK_LIBRARIES(${exe}
        Dbghelp winmm comctl32 dsound dxguid Version)

# 0CC-FamiTracker.rc includes res/0CC-FamiTracker.manifest.
# To prevent manifest linking errors:
# - res/0CC-FamiTracker.manifest MUST not be in add_executable().
# - /MANIFEST:NO must be passed into the linker.
set_property(
    TARGET ${exe}
    APPEND_STRING PROPERTY LINK_FLAGS
    " /MANIFEST:NO"
)

# Generating .pdb files
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(${exe} PRIVATE /Zi)

   # Tell linker to include symbol data
    set_property(
        TARGET ${exe}
        APPEND_STRING PROPERTY LINK_FLAGS
        " /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
    )

    # Set file name & location
    set_target_properties(${exe} PROPERTIES
        COMPILE_PDB_NAME ${exe}
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
endif()
