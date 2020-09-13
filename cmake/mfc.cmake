# taken from https://github.com/Kitware/CMake/blob/master/Tests/MFC/CMakeLists.txt.in
# The CMake source tree is distributed under terms of the BSD 3-Clause license.


macro(replace_flags var these those)
    if("${${var}}" MATCHES "${these}")
        string(REGEX REPLACE "${these}" "${those}" ${var} "${${var}}")
        #message(STATUS "info: ${var} changed to '${${var}}'")
    endif()
    message(STATUS "info: ${var}='${${var}}'")
endmacro()

macro(msvc_link_to_static_crt)
    if(MSVC)
        set(has_correct_flag 0)
        foreach(lang C CXX)
            foreach(suffix "" _DEBUG _MINSIZEREL _RELEASE _RELWITHDEBINFO)
                replace_flags("CMAKE_${lang}_FLAGS${suffix}" "/MD" "/MT")
                if(CMAKE_${lang}_FLAGS${suffix} MATCHES "/MT")
                    set(has_correct_flag 1)
                endif()
            endforeach()
        endforeach()
        if(NOT has_correct_flag)
            message(FATAL_ERROR "no CMAKE_*_FLAGS var contains /MT")
        endif()
    endif()
endmacro()


if ("${STATIC_MSVCRT}" STREQUAL "")
    message(FATAL_ERROR "Did not set STATIC_MSVCRT when including MFC CMake")
endif()

# First set static/dynamic MSVCRT...
if(STATIC_MSVCRT)
    msvc_link_to_static_crt()
else()
    # VS generators add this automatically based on the STATIC_MSVCRT value,
    # but generators matching "Make" require:
    add_definitions(-D_AFXDLL)
endif()

# Then call FindMFC.cmake (behavior depends on static/dynamic CRT).
FIND_PACKAGE(MFC)
IF (NOT MFC_FOUND)
    MESSAGE(FATAL_ERROR "MFC Could not be found during the MFC test")
ENDIF()

if("${STATIC_MSVCRT}" STREQUAL "2")
    set(CMAKE_INSTALL_MFC_LIBRARIES ON)
    include(InstallRequiredSystemLibraries)
endif()
