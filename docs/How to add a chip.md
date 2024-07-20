# How to add a Chip

This doc aims to detail how to add a custom chip to Dn-FT, should the
NSF/NSFe/NSF2/INES/INES2.0 update to add new expansion audio.

## Disclaimer

Dn-FamiTracker's aim is to fill in the gaps of NSF, NSFe and NSF2 features and/or emerging NESDev homebrew standards that 0CC or Vanilla fails to meet due to a lack of maintenance.

As such, Dn-FT will only implement features present in current NSF/NSFe/NSF2/INES/INES2.0 standards and definitions, keeping in mind the original goal of producing music for the NES/Famicom systems.

If you want to add different chips from different systems/formats, please direct your requests/efforts to [Furnace](https://github.com/tildearrow/furnace).


## 1. All of these **must be implemented in one go** before you can have a chance to prototype and debug:
1. Emulation core and emu core class handler. eg. `EPSM.cpp`; `emu_YMF288.cpp`
1. sound handling and mixing (eg. `APU.cpp`, `Mixer.cpp`, `SoundGen.cpp` implementations)
1. Channel handler (eg. `ChannelsEPSM.cpp`, `ChannelHandler.cpp`)
1. Instrument handling (eg. `InstrumentEPSM.cpp`, `InstrumentHandlerEPSM.cpp`, `InstrumentEditorEPSM.cpp`, `SeqInstHandlerEPSM.cpp`)
1. Document bindings and types (eg. `APU\Types.h`,  `ChannelFactory.cpp`, `ChannelMap.cpp`, `ChannelsDlg.cpp`, `DetuneDlg.cpp`, `FamiTrackerDoc.cpp`, `FamiTrackerTypes.cpp`, `FamiTrackerView.cpp`, `TrackerChannel.cpp`)

## 2. After prototyping and debugging and making sure everything works okay, be sure to implement the NSF driver
1. (see [bhop](https://github.com/zeta0134/bhop))