# TODO: https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/

cmake_minimum_required(VERSION 3.11)

set(project "Dn-FamiTracker")
set(exe "Dn-FamiTracker")
include(cmake_user_begin.cmake OPTIONAL)

project(${project})

# libsamplerate ships with 3 sinc tables of different qualities.
# SRC_SINC_BEST_QUALITY uses a massive 1.36 megabyte sinc table,
# and we don't even use SRC_SINC_BEST_QUALITY.
# So turn it off to save space in binaries.
set(LIBSAMPLERATE_ENABLE_SINC_BEST_CONVERTER FALSE CACHE BOOL "[libsamplerate] Enable Best Sinc Interpolator converter")

# MFC based off https://github.com/Kitware/CMake/blob/master/Tests/MFC/CMakeLists.txt
    # also sets https://stackoverflow.com/questions/14172856/cmake-mt-md
# Simpler approach (unimplemented): https://stackoverflow.com/questions/11580748/cmake-mfc
    # does not set mt vs md

# Configure MFC
if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    # Debug build requires dynamic MFC.
    set(STATIC_MSVCRT 0)
else ()
    # Otherwise link MFC statically.
    set(STATIC_MSVCRT 1)
endif ()
include(cmake/mfc.cmake)

# Acts like add_subdirectory(), but replaces Debug flags with RelWithDebugInfo flags.
# Use for unconditionally building performance-sensitive libraries in debug mode.
function(add_subdirectory_optimized)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    add_subdirectory(${ARGV})
endfunction()

# libsamplerate in debug mode burns lots of CPU,
# so build with optimizations on even in debug mode.
add_subdirectory_optimized("Source/libsamplerate" EXCLUDE_FROM_ALL)

# compiling
include(cmake/exe.cmake)

#builds the .chm file
if(WIN32)
    add_custom_command(TARGET ${exe}
        PRE_BUILD
        COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compile-chm.bat
        COMMAND move Dn-Famitracker.chm ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating help file...")
endif()

target_compile_features(${exe} PRIVATE cxx_std_17)

if(COMMAND target_precompile_headers)
    target_precompile_headers(${exe} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:Source/stdafx.cpp>"
    )
endif()


# linking
TARGET_LINK_LIBRARIES(${exe}
        Dbghelp winmm comctl32 Avrt Version htmlhelp
        samplerate)

# Dn-FamiTracker.rc includes res/Dn-FamiTracker.manifest.
# To prevent manifest linking errors:
# - res/Dn-FamiTracker.manifest MUST not be in add_executable().
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

include(cmake_user_end.cmake OPTIONAL)
