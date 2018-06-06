# TODO: https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/

cmake_minimum_required(VERSION 3.10)
project(j0CC_FamiTracker)
set(CMAKE_CXX_STANDARD 17)


# MFC debug/release
include(cmake/mfc.cmake)


# compiling
include(cmake/exe.cmake)


# linking
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        Dbghelp winmm comctl32 dsound dxguid Version)


# 0CC-FamiTracker.rc includes res/0CC-FamiTracker.manifest.
# To prevent manifest linking errors:
# - res/0CC-FamiTracker.manifest MUST not be in add_executable().
# - /MANIFEST:NO must be passed into the linker.

set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS
            /MANIFEST:NO)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
