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
# Debug build requires dynamic MFC.
# Otherwise link MFC statically.
set(STATIC_MSVCRT $<NOT:$<CONFIG:Debug>>)

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

find_package(Python 3.10)

# update version in Dn-FamiTracker.rc
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Dn-FamiTracker.rc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    COMMAND ${Python_EXECUTABLE} resource_version_update.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Updating version info in resource file."
    VERBATIM)

# generate HTMLDefines.h
set(DNHELP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp")
add_custom_command(
    OUTPUT  ${DNHELP_SRC_DIR}/HTMLDefines.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND call ${CMAKE_CURRENT_SOURCE_DIR}/generate-helpmap.bat
    COMMENT "Generating map file for help compiler."
    VERBATIM)

# build the .chm file
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${project}.chm
    DEPENDS ${DNHELP_SRC_DIR}/HTMLDefines.h
    WORKING_DIRECTORY ${DNHELP_SRC_DIR}
    COMMAND call ${DNHELP_SRC_DIR}/compile-chm.bat ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Making help file."
    VERBATIM)

# update changelog
add_custom_command(
    OUTPUT  ${DNHELP_SRC_DIR}/changelog.htm
            ${DNHELP_SRC_DIR}/license.htm
    DEPENDS ${DNHELP_SRC_DIR}/changelog-template.htm
            ${DNHELP_SRC_DIR}/license-template.htm
            CHANGELOG.md
            LICENSE.md
    WORKING_DIRECTORY ${DNHELP_SRC_DIR}
    COMMAND call ${DNHELP_SRC_DIR}/update-htm-templates
    COMMENT "Updating changelog.htm."
    VERBATIM)

# builds the NSF driver
set(DNNSF_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers")
add_custom_command(
    OUTPUT  ${DNNSF_SRC_DIR}/drv_2a03.h
            ${DNNSF_SRC_DIR}/drv_all.h
            ${DNNSF_SRC_DIR}/drv_fds.h
            ${DNNSF_SRC_DIR}/drv_mmc5.h
            ${DNNSF_SRC_DIR}/drv_n163.h
            ${DNNSF_SRC_DIR}/drv_s5b.h
            ${DNNSF_SRC_DIR}/drv_vrc6.h
            ${DNNSF_SRC_DIR}/drv_vrc7.h
    DEPENDS ${DNNSF_SRC_DIR}/asm/longbranch.mac
            ${DNNSF_SRC_DIR}/asm/init.s
            ${DNNSF_SRC_DIR}/asm/player.s
            ${DNNSF_SRC_DIR}/asm/effects.s
            ${DNNSF_SRC_DIR}/asm/instrument.s
            ${DNNSF_SRC_DIR}/asm/apu.s
            ${DNNSF_SRC_DIR}/asm/vrc6.s
            ${DNNSF_SRC_DIR}/asm/vrc7.s
            ${DNNSF_SRC_DIR}/asm/mmc5.s
            ${DNNSF_SRC_DIR}/asm/fds.s
            ${DNNSF_SRC_DIR}/asm/n163.s
            ${DNNSF_SRC_DIR}/asm/s5b.s
            ${DNNSF_SRC_DIR}/asm/periods.s
            ${DNNSF_SRC_DIR}/asm/vibrato.s
    COMMAND call ${DNNSF_SRC_DIR}/build.cmd
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building NSF driver headers."
    VERBATIM)

target_compile_features(${exe} PRIVATE cxx_std_17)

if(COMMAND target_precompile_headers)
    target_precompile_headers(${exe} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:Source/stdafx.cpp>"
    )
endif()


# linking
# Specify static MFC libraries nafxcw and Libcmt to avoid linker order issues
set(static_msvcrt_libs "nafxcw$<$<CONFIG:Debug>:d>;libcmt$<$<CONFIG:Debug>:d>")
set(dynamic_msvcrt_libs "msvcrt$<$<CONFIG:Debug>:d>")

target_link_libraries(${exe}
    $<IF:$<BOOL:STATIC_MSVCRT>,${static_msvcrt_libs},${dynamic_msvcrt_libs}>)

target_link_libraries(${exe}
    Dbghelp winmm comctl32 Avrt Version htmlhelp samplerate)

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
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND $<CONFIG:Release>)
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
