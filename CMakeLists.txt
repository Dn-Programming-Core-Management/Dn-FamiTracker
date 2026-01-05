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
add_custom_command(
	OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp/HTMLDefines.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND call ${CMAKE_CURRENT_SOURCE_DIR}/generate-helpmap.bat
	COMMENT "Generating map file for help compiler."
	VERBATIM)

# build the .chm file
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${project}.chm
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp/HTMLDefines.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp
	COMMAND call ${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp/compile-chm.bat ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Making help file."
	VERBATIM)

# update changelog
add_custom_command(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/Dn-help/hlp/changelog.htm
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp/changelog-shell.htm
			CHANGELOG.md
	COMMAND call ${CMAKE_CURRENT_SOURCE_DIR}/Dn-help/hlp/update-changelog-htm.bat
	COMMENT "Updating changelog.htm."
	VERBATIM)

# builds the NSF driver
add_custom_command(
	OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_2a03.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_all.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_fds.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_mmc5.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_n163.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_s5b.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_vrc6.h
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/drv_vrc7.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/longbranch.mac
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/init.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/player.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/effects.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/instrument.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/apu.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/vrc6.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/vrc7.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/mmc5.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/fds.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/n163.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/s5b.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/periods.s
			${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/asm/vibrato.s
	COMMAND call ${CMAKE_CURRENT_SOURCE_DIR}/Source/drivers/build.cmd
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
if (STATIC_MSVCRT)
	TARGET_LINK_LIBRARIES(${exe}
		Dbghelp winmm comctl32 Avrt Version htmlhelp
		samplerate nafxcw$<$<CONFIG:Debug>:d> libcmt$<$<CONFIG:Debug>:d>)
else ()
	TARGET_LINK_LIBRARIES(${exe}
		Dbghelp winmm comctl32 Avrt Version htmlhelp
		samplerate msvcrt$<$<CONFIG:Debug>:d>)
endif ()

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
