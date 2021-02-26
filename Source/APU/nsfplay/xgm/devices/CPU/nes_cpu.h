#ifndef _NES_CPU_H_
#define _NES_CPU_H_
#include "../device.h"

#define ILLEGAL_OPCODES 1
#define DISABLE_DECIMAL 1
#define USE_DIRECT_ZEROPAGE 0
#define USE_CALLBACK	1
#define USE_INLINEMMC 0
#define USE_USERPOINTER	1
#define External __inline

namespace xgm
{

/// Class NES_CPU has been stubbed out in exotracker (compared to nsfplay).
/// This is because exotracker does not emulate the NES 6502 CPU,
/// but (like Famitracker) implements sound driver logic (generates register writes)
/// using C++ host code.
///
/// Not emulating a 6502 may reduce host CPU usage compared to nsfplay.
///
/// Stubbing out the NES CPU reduces exotracker's dependencies on nsfplay,
/// simplifies exotracker code, and makes compiliation faster.
///
/// But it's difficult to delete NES_CPU
/// because NES_DMC and NES_MMC5 each hold a pointer to it.
///
/// - NES_DMC tells NES_CPU to delay reads, which is inconsequential.
/// - NES_MMC5 depends on NES_CPU::Read() for PCM playback
///   which FamiTracker does not support, and I may not either.
///   I may eventually edit NES_MMC5 to remove PCM playback
///   and not call NES_CPU::Read().
class NES_CPU
{
public:
  void StealCycles(unsigned int cycles) {}

  // IRQ devices
  enum {
    IRQD_FRAME = 0,
    IRQD_DMC = 1,
    IRQD_NSF2 = 2,
	IRQD_COUNT
  };
  void UpdateIRQ(int device, bool on) {}
};

} // namespace xgm
#endif
