#ifndef _DEBUGOUT_H_
#define _DEBUGOUT_H_

#ifdef _MSC_VER

// Prevent windows.h from redefining min and max.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

namespace xgm
{
#define DEBUG_OUT TRACE

  // overrides the next output sample (nsfplay.cpp)
  // use to mark events in WAV output, turn quality high
  // only works in _DEBUG build
  extern int debug_mark;
}

#else
#ifdef NDEBUG
#define DEBUG_OUT(...)
#else
#define DEBUG_OUT printf
#endif
#endif

#endif
