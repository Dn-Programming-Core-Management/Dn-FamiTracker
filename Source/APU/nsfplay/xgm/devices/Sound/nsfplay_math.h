#pragma once

#include "../../xtypes.h"  // UINT32
#include <algorithm>  // std::max
#include <cassert>

namespace xgm {
    inline UINT32 value_or(UINT32 x, UINT32 period) {
        // See https://docs.google.com/document/d/1BnXwR3Avol7S5YNa3d4duGdbI6GNMwuYWLHuYiMZh5Y/edit#heading=h.lnh9d8j1x3uc
        // for discussion on why this was designed to special-case 0.
        if (x == 0) {
            assert(period > 0);
            return std::max(period, (UINT32)1);
        }
        return x;
    }
}