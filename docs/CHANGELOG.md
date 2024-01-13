# Dn-FamiTracker Mod

Change Log

Written by D.P.C.M.

Version 0.5.0.2 - January 13, 2024

---

## Dn0.5.0.2 - 01/13/2024

- ### Important changes:

	- Modules will be saved as Dn-FT modules
	- Reintroduced JSON export
	- Update app icon to mimic Frutiger Aero/Y2K aesthetic

- ### Improvements:

	- Reintroduce JSON export from 0CC-FT (@nstbayless @Gumball2415 #197 #199)
	- Update application icon (@Gumball2415 #236)
	- Rewrite change log in markdown (@Gumball2415 #238)

- ### Bug fixes:

	- Fix effect number input using numpad (@ZeroJanitor @Gumball2415 #48 #214)
	- Force modules to be saved as Dn-FT modules (@Threxx11 @Gumball2415 #184 #214)
	- Fix outputting audio to multi-channel output devices (@CoolJosh3k @nyanpasu64 #205 #226)
	- Wait for APU mutex lock during .wav export (@nyanpasu64 @Gumball2415 #206 #214)
	- Avoid checking assert with unsigned integer cast (@freq-mod @N-SPC700 @Gumball2415 #209 #214)
	- Assert legacy mixing levels and ranges (@trashbinenthusiast @N-SPC700 @nyanpasu64 @Gumball2415 #213 #214)
	- Avoid division by zero in MML sequence parsing (@Gumball2415 #222 #214)
	- Fix incorrect speed in PAL NSF exports (@TakuikaNinja @eugene-s-nesdev @Gumball2415 #223 #242 #214)
	- Fix detune offset direction (@Gumball2415 #225 #214)
	- Disable Custom Exporter DLL loading (@eatscrayon @Gumball2415 #232 #214)
	- Fix access violation in MRU submenu list update (@eugene-s-nesdev @Gumball2415 #243 #214)

- ### Internal:

	- Fix version checker repository link (@Gumball2415 #229 #212)
	- Include logo and icon resources in the repository (@Gumball2415 #218 #236)
	- Add AddressSanitizer project configurations (@Gumball2415 #236)
	- Separate Github Actions binary downloads (@Gumball2415 #237)
	- Add automated draft release binary upload (@Gumball2415 #237)
	- Integrate Dn-help as submodule (@Gumball2415 #238)
	- Generate HTMLHelp manual change log on build time (@Gumball2415 #238)



## Dn0.5.0.1 - 05/10/2023

- ### Important changes:

	- Module breaking bugs have now been fixed. (@Gumball2415 #195)
	- Fix effects type order bug (@Threxx11 @Gumball2415 #184 #195)
	- Fix custom OPLL patch entry bug (@galap-1 @Gumball2415 #176 #195)
	- Fix N163 mixing bug (@recme @Gumball2415 #174 #195)

- ### Improvements:

	- Implement proper commandline support (@TheRealQuantam #168 #181)

- ### Bug fixes:

	- Fix NSF driver S5B envelope autoperiod (@N-SPC700 @Gumball2415 #186 #189)
	- Fix NSF driver arpeggio sequence note overflow (@N-SPC700 @Gumball2415 #189)
	- Fix NSF driver VRC7 note cut behavior to match in-tracker (@MovieMovies1 @Gumball2415 #189)
	- Fix effects type order bug (@Threxx11 @Gumball2415 #184 #195)
	- Fix custom OPLL patch entry bug (@galap-1 @Gumball2415 #176 #195)
	- Fix N163 mixing bug (@recme @Firespike33 @Gumball2415 #174 #195)
	- Fully initialize device level offset object (@Gumball2415 #195)
	- Prevent module compatibility mode reinitialization (@Threxx11 @Gumball2415 #184 #195)
	- Reinitialize OPLL patchset (@galap-1 @Gumball2415 #203 #195)
	- Initialize PlaybackRate and PlaybackRateType (@N-SPC700 @Gumball2415 #202 #195)

- ### Internal:

	- Update build tools, VS solution, CMake lists, and corresponding documentation (@Gumball2415 #186)
	- Add version increment documentation (@Gumball2415 #194)
	- Update VC++ program database file name in build script (@Gumball2415 #204)
	- Improve Appveyor build version info (@Gumball2415 #204)
	- Add Github Actions for build artifacts (@Gumball2415 #204)
	- Use Windows 10 SDK version 2104 (10.0.20348.0) to avoid unexpected linker failure (@Gumball2415 #204)



## Dn0.5.0.0 - 12/19/2022

- ### Important changes:

	- Dn-FamiTracker modules are no longer backwards compatible with "vanilla" and 0CC FamiTracker.
	- Dn-FamiTracker Demo Compo 1 modules are now added
	- N163 emulation core is replaced with Mesen's
	- New effects: =xx, and Nxy
	- Exports are now updated (text, NSF/NSFe, binary, asm)
	- NSF2 export added
	- External OPLL option (export only available for NSFe/NSF2)
	- Per-module expansion mixing (export only available for NSFe/NSF2)
	- Hardware-based module expansion mixing (export only available for NSFe/NSF2)
	- DirectSound backend now replaced with WASAPI

- ### Additions:

	- Add FDS waveform view (@EulousDev #113)
	- Add invalid note indicator (@EulousDev #119)
	- Add Dn-FT Demo Compo 1 Modules (@Gumball2415 #153)
	- Target volume slide effect (Nxy) (@ipidev #109)
	- Implement Kxx and =xx in NSF driver (@Gumball2415 nyanpasu64/j0CC-FamiTracker#122, #156)
	- Add supplemental NSF data on bin/asm export (NSF header, config, period tables and vibrato tables) (@Gumball2415 #156)
	- Add mixe chunk support and per-module device mixing (@Gumball2415 #68, #156)
	- Add hardware-based expansion audio mixing (@Gumball2415 #156)
	- Implement external OPLL patchset editing (@Gumball2415 #68, #156)
	- Implement VRC7 NSFe chunk (@Gumball2415 #156)

- ### Improvements:

	- Register view improvements (@Gumball2415 @nyanpasu64 @EulousDev #118, #120)
	- Replace N163 audio core with Mesen (@Gumball2415 nyanpasu64/j0CC-FamiTracker#151, #111, #138, #152)
	- Refactor FDS auto-FM overflow fix (@Gumball2415 #80, #65, nyanpasu64/j0CC-FamiTracker#133, #156)
	- Refactor Pxx overflow fix (@Gumball2415 #80, #156)
	- Refactor Gxx overflow fix (@Gumball2415 #80, nyanpasu64/j0CC-FamiTracker#129, #156)
	- Reset FDS modulator phase on new note (@Gumball2415 #156)
	- Refactor period table calculation (@Gumball2415 #156)
	- Update text import/export parity (@Gumball2415 #161, #162, #156)

- ### Bug fixes:

	- Prevent ctrl/shift+wheel edit outside edit mode (copyrat90 #87)
	- Fix random crash when exporting channels as WAV (@nyanpasu64 #92, #93)
	- Fix module reload on config confirmation (@Gumball2415 #89, #97)
	- Fix N163 volume meter level (@EulousDev #99, #103)
	- Fix VRC7 data race and use-after-free when reloading/closing modules (@nyanpasu64 #106)
	- Update source code copyright declaration and GPL 2.0+ notice (@Gumball2415 #116)
	- Fix wave export dialog refresh rate to GUI rate (@Gumball2415 #108)
	- Fix high-res spectrum visualizer at small buffer sizes (@nyanpasu64 #126)
	- Fix use-after-free race condition in visualizer (@nyanpasu64 #130)
	- Fix crash when starting program with pattern editor off-screen (@nyanpasu64 #128, #129)
	- Fix periodic noise showing cents when muted (@Gumball2415 #139)
	- Stop rejecting valid files using features missing from vanilla FT (@nyanpasu64 #150)
	- Fix N163 instrument pointer data overflow (@Gumball2415 #156)
	- Fix NSFe export from using incorrect DATA chunk size (@Gumball2415 #110, nyanpasu64/j0CC-FamiTracker#5, #156)
	- Fix 5B Wxx being overwritten by blank duty macros (@Gumball2415 #160, nyanpasu64/j0CC-FamiTracker#105, #156)
	- Fix N163 multiplex state desync (@Gumball2415 #163, #156)
	- Dn-FamiTracker modules are no longer backwards compatible with "vanilla" and 0CC (@Gumball2415 #156)
	- FamiTracker 0.5.0 beta modules no longer conflict with Dn-FamiTracker 0.5.0.0 modules (@Gumball2415 #156)

- ### Internal:

	- Revise README (@Gumball2415 #114)
	- Replace DirectSound backend with WASAPI (@nyanpasu64 #124)
	- Remove unused files, add headers to CMakeLists.txt (@nyanpasu64 #142)
	- Eliminate audio thread blocking on GUI messages and main thread (@nyanpasu64 #134, #137)
	- Add WASAPI resampling so all sampling rates work (@nyanpasu64 @Gumball2415 #143)
	- Fix crash when activating non-initial song then opening document with expansion chip(s) (@nyanpasu64 #147, #148)
	- Fix mostly-theoretical data race when popping from SPSC queues (@nyanpasu64 #149)
	- Properly terminate stuck audio thread when closing the program (@nyanpasu64 #155)



## Dn0.4.0.1 - 09/14/2021

- ### Patch fixes:

	- Fix internal version numbering (@Gumball2415)

## Dn0.4.0.0 - 09/13/2021

- ### Important changes:

	- New file extension format for modules: *.dnm (@Gumball2415 #71)
	- New file extension format for instruments: *.dni (@Gumball2415 #71)
	- Support for Windows XP has been dropped (@Gumball2415, @nyanpasu64, @N-SPC700 #82, #84)

- ### Additions:

	- Add new D.P.C.M. organization info, links and metadata (@Gumball2415 #71)
	- New instrument names are blank on creation (@Gumball2415)
	- Add empty instrument in newly created modules (@Gumball2415, @nyanpasu64, @N-SPC700 #77)

- ### Improvements:

	- Adjustable frame editor channel view limit (@Gumball2415 #72)
	- Fix frame editor channel view truncation (@Gumball2415 #72)

- ### Bug fixes:

	- Fix Gxx delay command overflow in NSF driver (@Gumball2415, @smbhacks #80)
	- Fix FDS automatic modulation overflow in NSF driver (@Gumball2415, @galap-1 #65, #80)
	- Fix period calculation overflow in NSF driver (@Gumball2415, @smbhacks #80)
	- Reset VRC7 emulator core on channel handler reset (@Gumball2415, @TakuikaNinja #79)
	- Flush file to disk before renaming to prevent module corruption (@Gumball2415, @nyanpasu64 #81)
	- Fix commandline parameter processing when flags are used (@Gumball2415)

- ### Internal:

	- Fix CString::Format() memory corruption bug on Wine (@nyanpasu64 #56)
	- Fix uninitialized echo buffer values (@nyanpasu64 #56)
	- Clarify nsfplay value_or() (@nyanpasu64 #56)
	- Clarify integer widths in CBookmark::Distance() (@nyanpasu64 #56)
	- Fix broken Open dialog after launching Dn by opening a file (@nyanpasu64 #74, #76)
	- Fix out-of-bounds read in oscilloscope (@nyanpasu64 #85)



## Dn0.3.1.0 - 06/30/2021

- ### Important changes:

	- N163 and 5B expansion audio mixing has been restored to 0.2.1 levels (@nyanpasu64 #66)
	- Adjusted APU 2 levels to match blargg's original formula (@Gumball2415 #69)

- ### Additions:

	- Added a textbox for patch editing within the VRC7 instrument editor (@Gumball2415 #60)

- ### Improvements:

	- Edit NSF export format to allow adding more effects (@Gumball2415, @nyanpasu64 #46)

- ### Bug fixes:

	- Fix VRC7 hardware patch bank presets not reading properly (@Gumball2415 #60)
	- Fix crash when increasing engine speed with VRC7 enabled (@nyanpasu64 #62)
	- Fix N163 and 5B mixing levels which were swapped in 0.3.0 (causing N163 to be louder and 5B to be quieter) (@nyanpasu64 #66)

- ### Internal:

	- Update VRC7 emulator core (emu2413) to v1.5.6 (@Gumball2415 #59)
	- Port CVRC7 to CSoundChip2 (@Gumball2415 #59)
	- Refactor VRC7 hardware patch bank swapping (@Gumball2415 #59)



## Dn0.3.0.0 - 04/03/2021

- ### Important changes:

	- Reenabled version checking, which can also be accessed in the Help popup
	- Replaced FDS emulation core with Mesen's version (@nyanpasu64 #32)
	- Replaced 2A03 emulation core with NSFPlay's version (@nyanpasu64 #32)
	- Added back the help manual, with additional updated info contributed by:
		- Blue Mario for Sunsoft 5B information (@N-SPC700 #12)
		- Accuracy (Compass Man) for providing updated screenshots (#12)

- ### Additions:

	- Added FastTracker 2 (JP106) keymap (@sdhizumi #8)
	- Adjustable idle refresh rate in configuration
	- Emulation tab in Configuration, which currently includes:
		- Toggleable N163 multiplexing
		- Swappable VRC7 hardware patches
		- Adjustable FDS cutoff filter (@nyanpasu64 #42)
	- Added the Kxx multiply frequency effect, not currently supported in NSF export @nyanpasu64 #16)
	- Adjustable channel view in frame editor

- ### Improvements:

	- Enable DPI scaling for pattern editor (@nyanpasu64 #5)
	- Updated About information
	- Export wave shortcut (Ctrl + Shift + E)

- ### Bug fixes:

	- Fixed DC drifting (when no highpass is applied) due to rounding errors in Blip_Buffer calculation (@nyanpasu64 #27)
	- Fixed N163 detuning when multiplexing is disabled (@nyanpasu64 #31)
	- Fixed undoing "delete frame" adding a new frame (#43)
	- Fixed VRC7 note cuts being inconsistent with in-tracker note cuts (@Kouzeru #47)

- ### Internal:

	- Fixed CMake MFC detection and link flag conflicts (@nyanpasu64 #6)
	- Removed more occurences of old program name and renamed more files to Dn (@nyanpasu64 #7)
	- Replaced the application icon (design by Accuracy (Compass Man))
	- Enabled x64 build compiling in the Visual Studio files
	- Updated the VRC7 emulation core to emu2413 v1.5.2
	- Include changelogs of vanilla, j0CC, and Dn in changelog.txt
	- Moved resource.h and Dn-FamiTracker.rc back to root folder due to HTMLHelp compilation
	- Corrected paths on case-sensitive filesystems (@nyanpasu64 #18)
	- Removed header files from CMake (@nyanpasu64 #26)
	- Replaced Blip_Buffer with an improved fork (@nyanpasu64 #27)
	- Added emulation clock-skipping until next level change to save CPU, currently implemented in 2A03 and FDS emulator cores (@nyanpasu64 #32)
	- Fixed file extension association in Dn-FamiTracker.reg (@Raphaelo24 #36)



## Dn0.2.1.0 - 09/13/20

- ### Additions:

	- Adjustable preview pitch.

- ### Improvements:

	- Readjusted the DPCM sample editor dialog size limit.

- ### Bug fixes:

	- Fixed drag-scrolling not working.
	- Readjusted the DPCM sample editor dialog size limit.

- ### Internal:

	- Changed the project and corresponding project file names accordingly to Dn-FamiTracker.



## Dn0.2.0.0 - 09/07/20

- ### Additions:

	- Multiple .wav per-channel export (#2)
	- Adaptive register state refresh rate(≈60fps during playback, otherwise 10fps)

- ### Bug fixes:

	- DPCM sample bit reversal now includes the first bit of the sample
	- Fixed the version numbering

- ### Internal:

	- Changed most of the internal names from j0CC-FamiTracker to Dn-FamiTracker
	- Use precompiled headers in CMake builds (@nyanpasu64 #3)



## Dn0.1.0.0 - 08/05/20

- ### Additions:

	- DPCM sample bit order reversal (located in the DPCM sample editor) (#1)

---



# j0CC-FamiTracker Mod

Change Log

Written by nyanpasu64

Version 0.6.3 - August 1 2020


---

## 0.6.3 - 08/01/20

This will be the final release. This program has been effectively dead for
months to years, as my priorities have moved elsewhere, to building a new
tracker from scratch, freed from MFC and being chained to Win32, freed from
DirectSound and 40-70 ms of audio latency, freed from a fixed row grid that
falls apart as soon as you try to use triplets...

Additionally I can't let this program continue under its current name. I can't
have people talking to me about the program under its current name. I should've
renamed the program earlier, but never picked one.

As a parting gift, I've attached a 64-bit build from CMake as well. This may be
compatible with Wine on macOS Catalina (though you'll have to either build Wine
yourself or look for prebuilt packages, maybe
https://www.reddit.com/r/wine_gaming/comments/er28u5/is_there_anyway_to_run_wine_in_macos_catalina/ff14rns/ ).

I invite the community to fork the program, possibly under a name like
FamiTracker-Next. I may participate if I regain interest.

- Fix 5B instrument recording (#138, @Teuthida255)
- Re-add ability to disable "retrieve channel state" (#146)
- Remove popup message box when enabling linear pitch mode (#144)
- Removed 'Transpose Song' instrument check (#147, @Teuthida255)
- Refactor sound output code (DirectSound code) (#143)
- Remove build dependency on DXSDK and dxguid.lib (#160)
- Enable 64-bit builds (#154)
- Fix memory corruption on Wine during find-and-replace (#164)
- Fix out-of-bounds read (garbage data or crashes) with N163 Vxx effects over 0x80 (#165)

:door::cat2:



## j0.6.2 - 06/16/19

the version number in the .exe is wrong. Ignore it.

- Update VRC7 preset patches (#135) (by @Teuthida255)
- Fix crash when pasting large selections near the rightmost channel (#137)

See https://ci.appveyor.com/project/nyanpasu64/0cc-famitracker/history for dev
builds.

## j0.6.1 - 09/30/18

Bugfixes:

- Fix crash when saving files on Wine.
- Fix playback/WAV export desync, when FDS channel is enabled.
	- Every instrument switch would cause the frame to take longer than usual.

Internal:

- Globally renamed application from 0CC-FamiTracker to j0CC-FamiTracker.



## j0.6.0a - 07/08/18

j0.6.0a is identical to j0.6.0, except with correct .exe version number, and
.pdb debug info available in release builds, to help debug crash dumps.

Changelog:

- Add =00 effect (VRC6 phase reset) (no effect on NSF)
- Add configurable font size scaling (without changing row height)
- Always enable Retrieve Channel State (remove config option)
- Every time you begin playback, j0CC loads the current volume and active effects.
- Validate Vxx and DPCM (Zxx) effects, mark out-of-bounds parameters as red
- eg. V04 and above in 2A03, Z80 and above in DPCM
- Add confirmation dialog when clearing recent files
- Interface text improvements to improve information and clarity

Bugfixes:

- Fixed 5B noise bug where non-noise instruments overwrite noise period (0CC playback)
- Fixed 5B noise-period envelopes when exporting NSF files
- Note: 5B Wxx effect (noise period) has no effect in NSF
- Fix theme color importing bug, where "Highlighted background 2" would overwrite "Highlighted background" (also "Highlighted pattern text 2")
- Fix crash when cancelling "export to WAV" dialog

Internal:

- Now generates .pdb debug info for release builds, to help debug crash dumps.
- CLion (CMake) build support
- Move NSF driver source into this repository, add build script
- To reduce repository clutter, move resource.h and 0CC-FamiTracker.rc to res/



## j0.5.3 - 06/01/18

Changelog:

- Improved descriptions for 1xx and 2xx effects.

Bugfixes:

- Fix bug where Xxx effect (DPCM retrigger) waited xx frames for first retrigger, subsequently xx+1 frames.
	- Still broken in NSF export
- Center N163 output about sample=8, when volumes change (hardware-accurate).
	- Previously centered about sample=0, creating loud pops when changing volume)
- Fixed bug where VRC7 Ixx (custom instrument) effect didn't work, when Channel 1 contains default instruments.
	- Not broken in NSF export
- Fixed bug where N163 Zxx (wave position) effects would fail when switching instruments. (Removed Zxx error checking, since it checked the old wave size)
- Fixed bug where "Ignore Step when moving" wouldn't take effect until changing step size.



## j0.0.5.2 - 04/29/18

Changelog:

- FTM, NSF, and text exports share "most recent path".

Bugfixes:

- Reset N163 level offset in sound engine, when switching files.
- N163 dialog no longer crashes program, when pasting >64 waves.



## j0.0.5.1 - 04/16/18

Changelog:

- Fix bookmark "sort by position" (from HertzDevil)
- Enable "Retrieve channel state" as default for new users.
- Fix default FDS sine modulator, so it plays properly on NSF.

See https://ci.appveyor.com/project/nyanpasu64/0cc-famitracker/history for past builds.



## j0.0.5 - 04/13/18

Changelog:

- N163 file-specific mixing level offset.
- Enable through the Module Properties dialog (Ctrl+P).
- Typing Pxx (or FDS Zxx) defaults to P80.
- Enable ASLR and DEP/NX (for added "security").

Bugfixes:

- Save TXT export path properly.
- Fix color scheme import.
 
See https://ci.appveyor.com/project/nyanpasu64/0cc-famitracker/history for dev builds.



## j0.0.4 - 03/22/18

Changelog:

- Reinstate high-resolution FFT display, without crashing.
- Add placeholder tips to "find and replace" dialog.
- Warn user when replacing with Edit Mode disabled.

Bug fixes:

- Fix crash on refresh rates below 25 FPS.
- Fix configuration menu not applying changes.



## j0.0.3 - 03/02/18

Disabled update checking (may reenable checking this repo later). Changed the version number.

Fixed #10 and #11 for Wine users.



## j0.0.2 - 03/02/18

Fixed a crashing issue in debug builds, and reported to occur in release builds. (Caused by hires FFT)



## j0.0.1 - 03/02/18

This is a fork of HertzDevil's 0CC-Famitracker 0.3.14.5 (since 0.3.15.1 and master are quite buggy and changing rapidly). It contains bugfixes which HertzDevil has not merged yet (some for months, some fixed independently in 0.3.15.1 or master), as well as N163 multi-wave copy-paste support.

Bugfixes:

- Don't corrupt memory when entering MML volume sequences over 252 items long (instead truncate).
- Fix bug where find-replacing anything with an empty effect creates " 00" effect.
- Fix text import instrument loop/release (@owomomo, fixed in 0.3.15.1).
- Update channel count after importing text (like master). Mark file as modified.

Enhancements:

- N163 wave editor's copy/paste buttons copy all waves at once, separated/terminated with semicolons. This allows for highly efficient Audacity-N163 import workflows (see https://gist.github.com/nyanpasu64/424110eab84dad50cf1a6646a72b2627).
- Hi-res FFT spectrogram (like master).



## n163-2018-02-10 - 02/12/18

My changes:
- Fixed importing FTI instruments containing DPCM samples where length's bytes >= 0x80. This fixes Extends Levant bass.
- Fix memory corruption when entering MML volume sequences over 252 items long.
- Modify N163 dialog buttons to copy/paste all waveforms at once, separated by  semicolons. A very useful import format for Audacity wavetable exporting,  and copy-pasting between instruments.

Bugfixes from HertzDevil:
- Changing channels does not clear bookmarks.
- Changing grooves does not crash (I think).
- RAM usage decreased for large files.

Known bugs:
- Oscilloscope is partially flatlined.
- Setting engine speeds <= 24 Hz causes memory corruption and crashes. (long-standing bug)
- Clearing File/Recent Files results in memory corruption, crashing, and system freezes.



---



# 0CC-FamiTracker Mod

Change Log

Written by HertzDevil

Version 0.3.15.3 - May 20 2018

---

## V0.3.15.3 - 05/20/18

Bug fixes:

- Fixed a fatal issue that caused drag and paste operations to create incorrect
   pattern selections



## V0.3.15.2 - 05/20/18

Additions:

- Added JSON export (experimental)
- Arpeggio scheme sequence editor now includes a mode selector, use left / right mouse button to cycle forward / backward
- Module import options for instruments are grooves are now more granular:
	- Do not import
	- Duplicate and import (same as existing behaviour, indices fill empty slots)
	- Import and overwrite (resources from new module replace existing resources,   indices do not change)
	- Import missing entries (similar to above, but import only if existing slot   is not occupied)

Improvements:

- Tracker now uses UTF-8 encoding, this also fixes garbled text at various places
- Tracker now retrieves song state when playing a single row
- The following actions are now undoable:
	- Add / remove / rename instrument
	- Swap instruments
	- Clear patterns
- CPU usage display on the performance dialog is now more accurate
- Changing frame count no longer deletes bookmarks beyond the last frame, they are now greyed out on the bookmark settings dialog
- Add / Remove DPCM assignment buttons on 2A03 instrument editor now work across octaves
- NSF export no longer modifies current document
- Sequence editor now draws straight lines when moving cursor across multiple items at once
- Clicking loop or release point on the sequence editor now removes the point on the sequence
- 5B noise sequence editor is now closer to 0.5.0 beta's implementation
- Fxx speed / tempo split point is now adjustable on the module properties dialog
- DPCM samples and instrument sequences are no longer imported if instruments are not imported
- "Spectrum analyzer (fine)" visualizer is now finer than before
- Replaced FDS emulation core with NSFPlay's upstream version
- Replaced Fast Fourier Transform code with new one licensed under MIT

Bug fixes:

- Improved tracker responsiveness when scrolling across multiple patterns
- Fixed unused patterns claiming memory unnecessarily
- Tracker no longer freezes when searching for a non-existent groove index less than the current groove index on the song settings panel
- Fixed crash when loading instruments while importing text files
- Fixed crashes when importing FTIs for sequence instruments
- Fixed text exporter writing additional text for FDS instruments
- Tracker now reads N163 FTIs properly that include auto-wave position information from 0.5.0 beta (but still does not implement it)
- Fixed find / replace dialog not recognizing hexadecimal strings
- BPM display and wave file renderer no longer run twice as fast
- Drag and drop across tracker instances now displays the correct number of rows in the selection
- Fixed tracker erasing pattern selection after dragging and dropping to the same location
- Theme import / export now handle hexadecimal strings properly
- Fixed "Clear All" on groove dialog removing groove 1F from the list
- Fixed "Auto arpeggiate chords" option not working when current song is not playing
- Fixed row highlight sometimes not aligning with first row of the current bookmark region / beginning of the song
- Fixed swap channels dialog sometimes not allowing swap when 2A03 is selected
- Fixed several incomplete error messages when loading invalid modules
- Fixed VRC7 Hxx and Ixx not working if a non-custom patch is played before a custom patch
- VRC7 volume meter no longer flickers on "fast" decay rate
- Fixed instrument recorder behaviour on VRC6 PUlse 2
- Fixed "x:y" and "x:y:z" syntax not working with note names for fixed arpeggio MML input
- Version checker now always looks for the most recent version number
- Player no longer retrieves song state of current row to be played



## V0.3.15.1 - 10/30/17

Bug fixes:
- Fixed pattern block corruption for songs containing more than 256 rows



## V0.3.15.0 - 10/30/17

Important Changes:
- Custom exporter support has been dropped because of lack of use

Additions:
- Added the ability to render WAV files from the command line

Improvements:
- The following actions are now undoable: - Changing title / artist / copyright fields - Changing comment settings - "Populate Unique Patterns"
- Player now moves to the queued frame if it loops the current pattern
- Saved modules will now completely skip empty data blocks

Bug fixes:
- Added workaround for Wine in the instrument editor and the configuration menu
- Fixed crash from using "x:0" or "x:0:z" entries in sequence editor MML fields
- Changing to a new song now resets the row marker
- Tracker no longer queues notes to other channels if auto-arpeggio is enabled
- Fixed forum link display on the About dialog
- Reverted a change that caused new notes on N163 channels to sometimes not reload the wave with the correct indices
- Exporting multi-chip NSFs no longer modifies the current module



## V0.3.14.5 - 01/16/17

Important Changes:
- Effect indices are now remapped to match new effects in vanilla 0.5.0 beta
- 5B implementation is basically complete, so previous 0CC-FT 5B modules may become incompatible

Additions:
- Ported the following features from 0.5.0 beta: - Reading 0.5.0 beta FTI instrument files - VRC7 Hxx / Ixx / Vxx pattern effects - 5B Wxx pattern effect
- Added one-past-the-end frame editor row

Improvements:
- Swapped 5B Hxy / Jxx and inverted noise pitch values to match 0.5.0 beta
- 5B Wxx effect can be used on any 5B channel
- 5B automatic envelope frequency now rounds off instead of rounds down
- Exported multichip NSFs no longer contain data for unused sound chips
- Changes to pattern indices now respect active frame selections
- Creating a frame selection now always sets the focus to the frame editor
- BPM display now follows current highlight settings
- Groove settings dialog now shows a "*" for non-empty grooves

Bug fixes:
- Fixed tracker failing to read registry settings if the key "FamiTracker" does not exist
- Fixed crashes due to deleting instruments while they are being used by the player
- Fixed tracker crashing when reading from any 0.5.0 beta module containing note data on row 0x80 or below
- Fixed VRC7 channels not resetting the pitch after note cuts during an active 3xx command
- Fixed groove settings dialog not restoring previous values after selecting "Cancel"
- Instrument editor now displays "-y" properly for arpeggio schemes
- Fixed undo operations for changing pattern indices
- Fixed Ctrl / Shift + Mouse Wheel not working without an active selection in the pattern editor while playing with follow mode disabled
- Fixed "Populate Unique Patterns" command and changes to expansion chip configuration erasing song names



## V0.3.14.4 - 08/04/16

Bug fixes:
- Fixed recent file list not updating the registry if cleared without being viewed again at least once
- Fixed Vxx on blank instruments being overridden by instrument duty sequences
- Fixed most bugs related to portamento effects on the VRC7 channels in the tracker



## V0.3.14.3 - 08/03/16

Additions:
- Ported the following features from 0.5.0 beta:
	- Reading 0.5.0 beta FTMs and importing 0.5.0 beta text files
	- 5B Hxx / Ixx / Jxy pattern effects (H and J swapped according to 0CC-FT)
	- && hold instrument command
	- Absolute instrument pitch sequences
	- 64-step instrument volume sequences for VRC6 sawtooth
	- Module global tuning
	- Player bookmark (as "row marker")

Improvements:
- Ported the following changes from 0.5.0 beta:
	- Sunsoft 5B code using Blip_Buffer
	- MML field entry for 5B noise / mode sequences
	- Removing octave setting in DPCM assignment editor
	- Average BPM display
	- [WIP] Fast volume meter decay rate
	- Saving / loading themes
	- Reset button in mixer configuration menu
	- Registry entry for frame editor font
	- Movable toolbars
- New tracker identity:
	- Tracker now fully uses "0CC-FamiTracker" as application title and uses its own registry key (loads default values from official FamiTracker)
	- New application icon and document icon
	- New visualizer display
- Added support for DPI scaling
- Selections in the frame editor now have channel scope
- Added a command to move the current selection to the active editor window
- "Paste & Duplicate" and drag & drop cloning for the frame editor now reuse common pattern indices within selection
- Overwrite pasting and range selection commands now work in the frame editor
- Added a pseudo-term "\$\$" for MML fields which treats all subsequent terms as hexadecimal even without an explicit "$" or "x" prefix
- Fixed arpeggio sequences may accept note names in the MML field
- Added buttons to toggle 5B envelope output in the graph editor for 5B noise / mode sequences
- Redone detune dialog
- "Strict" is now above "Official" for module error levels

Bug fixes:
- Fixed tracker crashing when retrieving the state of a non-existent groove
- Fixed pattern editor not allowing C-0 in polyphonic note preview
- Fixed pattern editor not moving to the new frame after dragging a selection outside the current frame
- Fixed "Left Edge" and "Right Edge" on the pattern editor's horizontal scroll bar
- Fixed tracker using parent directory as last used folder after loading multiple instrument files or DPCM samples
- Fixed N163 instrument wave editor displaying 40 as the wave size if it used a wave size of 4
- Fixed C-1 and C#1 on the search query's note field matching note release and note cut respectively
- Fixed search wildcards not working for insturment and volume fields
- Fixed search results dialog moving the cursor to the incorrect channel for expansion chips
- Fixed DPCM channel playing note C-0 if it contains an active note after retrieving the channel state
- Fixed "Recall channel state" command crashing on retrieving the state of a non-existent groove
- Fixed instrument recorder not working for the FDS channel
- Exx effect now works on the triangle channel
- Fixed VRC6 sawtooth outputting sound in exported NSFs when the instrument volume is 0
- Fixed VRC7 using an incorrect pitch lookup table in exported NSFs
- Fixed VRC7 not making any sound in exported NSFs after using a note release



## V0.3.14.2 - 05/23/16

Additions:
- Added linear pitch mode (originally from official 0.4.0)
- Added split keyboard settings dialog

Improvements:
- Keyboard input now queues notes to different channels of identical type when edit mode is disabled
- Undoable actions now remember the selection information immediately before and after performing the action
- Added "Find All", "Find Previous" and "Replace Previous" commands
- Added "Selection" as the find / replace scope
- Changed "Any" effect column scope to "All"
- "Replace All" and changes to highlight settings are now undoable
- Register state now uses colours for N163 waveform display
- Module properties dialog can now insert songs at current position or delete multiple songs
- Added configuration settings item to check for new versions on startup
- [WIP] Added module error levels
- Shortcuts menu now checks for conflicts between shortcut commands
- Alert prompts for non-undoable commands now select "No" as the default option
- Added menu command to clear the most recently used file list
- Added shortcut to select instrument by typing instrument number

Bug fixes:
- Fixed file save dialog not displaying .ftm files when saving modified untitled files while loading another document
- Undoable actions no longer overwrite the redo state information every time they are undone
- Transposing and scrolling pattern values no longer create a selection under the cursor
- Fixed Insert and Backspace keys not working properly if an active selection spans across multiple frames or does not begin in the current frame
- Fixed tracker resetting bookmark positions after moving frame up or down
- Fixed repeat key inputting invalid notes after using ^0 echo buffer access note
- Fixed find query not working if note field contains only note name
- Fixed replacement query not allowing note cut or note release in the note field
- Fixed find / replace dialog replacing current note if the search query is modified after the previous query matched
- Fixed performance dialog not displaying after opening once and then closing
- Fixed tracker not stopping notes from keyboard input if octave is changed while depressing keyboard key
- Fixed 2A03 channels not using PAL period table when PAL is used
- Fixed DPCM channel not playing if a VRC6 or 5B instrument is loaded as the current instrument
- Fixed MMC5 channels not playing sound when switching to another song where the period high byte remains unchanged
- Fixed VRC6 pulse channels and 5B channels using incorrect period tables for the instrument recorder
- DPCM data always outputs to same directory as music data instead of tracker root when exporting BIN data and current module contains no DPCM samples
- Fixed 3xx effect commands causing pitch bends when issued no notes are being played on the channel
- Fixed 3xx effect commands on VRC7 channels ignoring the current octave register value
- Fixed N163 loading the waveform of the currently selected instrument if the channel has not used any instrument
- Fixed VRC7 channels inverting the 4xy vibrato pitch offset in the tracker



## V0.3.14.1 - 04/09/16

Improvements:
- Tracker now pads DPCM samples with invalid size on opening a module
- Added shortcuts to toggle or solo the currently selected sound chip
- Vxx effect commands on muted N163 channels now write to the wave buffer when retrieving channel state
- Noise channel now performs slide effects with no pitch limits
- Cxx effect now finishes playing the current row before halting
- The 2A03 frame counter is now clocked at exactly 240 Hz (slightly more accurate than before)

Bug fixes:
- Tracker no longer validates invalid notes (according to official build)
- Fixed frame editor not moving the cursor to the clicked channel when clicking on its left/right edges
- Fixed module properties dialog not moving bookmark lists when moving songs up or down
- Fixed transpose dialog crashing on selecting "Transpose all tracks"
- Fixed tracker not updating the vibrato table after loading a new module
- Changing the module's highlight settings or vibrato style is now recognized as a file change
- Fixed DPCM editor not saving changes
- Fixed VRC7 and 5B not using the refresh rate of the current module
- Fixed tracker not producing sound after rendering a WAV file until player begins
- Fixed WAV render omitting last tick when rendering by a time amount
- Fixed find dialog not working properly when some ranges are omitted
- Fixed FDS and MMC5 not producing sound in exported multichip NSFs unless both are enabled
- Fixed relative instrument arpeggio sequences clipping the channel note at A#7 in exported NSFs
- Fixed noise channel clipping at 0-# of an octave below in exported NSFs when using download portamentos
- N163 channels no longer update the wave buffer on every frame
- Fixed new notes on N163 channels not updating the wave buffer in exported NSFs if instrument does not use a wave instrument sequence



## V0.3.14.0 - 03/31/16

Additions:
- Added the *.0cc file extension
- Added diagnostic information when loading invalid modules
- Added transpose dialog

Improvements:
- 2A03 instruments now implement the instrument interface
- FDS instruments are compatible with other channels (as untyped sequence instruments)
- Multichip NSFs now skip writing to registers of unused sound chips
- Implemented FDS Zxx in exported NSFs
- Reordered main frame menu for clearer organization
- Re-added fine spectrum visualizer

Bug fixes:
- Fixed document wrapper for custom exporters crashing when retrieving a non- existent instrument
- Fixed tracker crashing after importing any FTM containing bookmarks
- Fixed custom exporters crashing due to incompatible interface layouts
- Fixed DPCM functions in document wrapper crashing the tracker if the queried sample does not exist
- Fixed certain multichip exported NSFs crashing due to VRC6 overwriting the driver code in presence of FDS
- Fixed tracker ignoring "Backup files" option
- Fixed find / replace dialog not clearing wildcard effect name cache on new searches
- Selection now always moves to current frame if multi-frame selection is disabled (according to official build)
- Fixed selection with Shift + arrow keys allowing selection across frame boundaries when option is disabled
- Fixed Shift + arrow keys not selecting entire rows when compact mode is enabled
- Fixed selection errors when overflow paste mode is disabled
- Fixed 2A03 instrument not copying delta offset values on cloning
- Fixed BPM display showing values above maximum tempo at current refresh rate
- Fixed VRC6 channels sometimes generating an extra click on starting



## V0.3.13 - 11/16/15

Additions:
- Added a command to create new instruments by recording the output of certain channels as instrument sequences
- Instruments of all sound chips (except FDS and VRC7) are compatible

Improvements:
- "Retrieve channel state" now respects Fxx and Oxx effects, as well as current groove position
- Tracker now stops retrieving channel states after encountering a Cxx effect
- Added ranges to the search query and the ability to search fields that do not match parts of the query
- Pressing a note key always previews the current instrument even when cursor is not on the note column when edit mode is disabled
- Ctrl + Left/Right now shifts the phase of the current wave in the FDS / N163 wave editor, Ctrl + Down inverts the wave

Bug fixes:
- [11/30/15 interim build] A00 command now updates the channel default volume for subsequent Axy and Mxy effects
- [11/28/15 interim build] Multichip NSFs no longer crash due to FDS allowing VRC6 and VRC7 to modify the driver code
- [11/26/15 interim build] Fixed VRC6 and 5B channels not silencing when the insturment sequence volume is 0 in exported NSFs
- [11/21/15 interim build] Fixed instrument recorder crashing when a new FTM is loaded
- Cleanup actions that alter the pattern data always reset the undo history
- Fixed various memory leaks
- Copying selection as plain text now empties unselected columns properly
- Text export now writes unused N163 channels to output file according to the behaviour of official build
- Fixed text importer not allowing "^-0" in pattern note field
- Added ".-#" wildcard for search query for the noise channel
- Fixed horizontal tab character not working in sequence editor MML field
- Fixed arpeggio scheme text field parsing "+x y..." as "+y y..."
- Fixed FDS instrument editor registering a file change on opening if the current instrument uses any non-zero FM parameter
- 7xy effect on VRC6 sawtooth channel matches NSF behaviour (clip at volume 34 if V01 is set and volume underflows)
- N163 Z7F command now takes effect immediately
- Fixed incorrect slide effect parameters from recalling N163 channel state
- Fixed Zxx on N163 channels in exported NSFs being cancelled by loading new instruments
- Fixed MMC5 Pulse 2 not halting when MMC5 Pulse 1 is muted in multichip NSFs
- Fixed the above bug crashing NSFs containing the FDS chip
- Fixed 1xx, 2xx, 3xx effects becoming 1/4 as fast on N163 in exported NSFs



## V0.3.12 - 10/17/15

Additions:
- Added FDS automatic frequency modulation effects
- Bookmark manager no longer crashes
- Added a menu command to swap pattern data of two channels in the FTM

Improvements:
- Added a menu command that displays the estimated song duration
- Pattern editor does not play notes from keyboard input while playing with edit mode enabled and follow mode disabled
- Selections in compact view always include all columns in selected channels
- Arpeggio scheme sequence editor now displays the arpeggio type of the term below the mouse cursor properly
- Arpeggio scheme sequence editor can now set the arpeggio type from the graph editor (by holding numpad 0 - 3)
- Register display now highlights recent register writes, blue for new values, red for old values
- Groove settings dialog allows direct text input and copying grooves as Fxx effect pattern data
- Added a context menu command for the VRC7 instrument editor to copy the current instrument as plain text values

Bug fixes:
- Tracker no longer crashes upon loading FTMs that contain patterns with zero non-empty rows
- Tracks using fixed tempo now always allow speed values up to 255
- Fixed pattern editor not resetting the selection area when using Shift key to create a new selection
- Fixed pattern editor not resetting the selection area after cursor crosses frames while playing with follow mode enabled
- Fixed tracker not moving bookmark tables appropriately after tracks are moved or removed on the track list
- Fixed text importer erroneously enabling default groove for random tracks
- Fixed "Any" as effect column scope not working in Find / Replace dialog
- Fixed Module Properties dialog removing the FDS channel upon confirmation
- Fixed Module Properties removing channels to the right of N163 when changing only the number of N163 channels
- Fixed 2A03's nonlinear mixing sometimes applying even while channel is muted
- Fixed inactive N163 channels loading the waveform of the currently selected instrument upon playing
- Fixed wave export rendering extra rows upon encountering skip effects
- Fixed VRC7 instrument editor showing -6 dB / oct instead of -3 dB / oct for the rate scale settings
- Fixed 2A03 Pulse 1's state not being retrieved
- Fixed NSFs with certain expansion chip configurations crashing
- Fixed Axy and Mxy resetting the channel volume incorrectly
- Fixed N163 Zxx effect not setting the correct wave position in exported NSFs



## V0.3.11r1 - 07/12/15

Improvements:
- Added simple wildcard for search query ("." character)

Bug fixes:
- Pasting single-channel pattern data that begins from a channel volume column no longer moves the target selection area depending on the cursor position
- Pasting single-channel effect data no longer moves the target selection area if the cursor is outside the effect columns
- Fixed paste undo applying to as many effect channels as 2A03 Pulse 1 has for single-channel effect clip data
- Fixed Find / Replace dialog crashing upon any action
- Effect search queries without effect parameter now work properly
- Fixed Mxy effect not resetting to the default channel volume in exported NSFs
- Fixed ^0 echo buffer access retrieving incorrect notes outside 2A03 Pulse 1 in exported NSFs



## V0.3.11 - 07/09/15

Additions:
- Added pattern bookmark manager and go to dialog

Improvements:
- Added a visual waveform manager for the N163 instrument editor
- Added an option in the configuration menu to disable selections across frames
- FTM import now allows importing FTMs with different expansion chips
- "Pick Up Row" now updates the last used values of all columns for repeat key
- "Copy As Plain Text" now includes header row containing channel names
- "Recall Channel State" now respects several run-time effects while playing
- Loop points of instrument sequences can be placed after release points

Bug fixes:
- Tracker can no longer open more than one performance dialog or groove settings dialog
- Added temporary fix for the tracker not setting up painting buffers properly after resizing
- Fixed pattern operations not working properly when the current selection spans across any frame with only one row
- Fixed several pattern operations improperly applying undo to the first row of the track
- Fixed stretching, transposition, and Shift + Mouse Wheel overwriting itself incorrectly for selections spanning across truncated frames
- Fixed "Copy As Plain Text" removing first few columns from the first channel
- Implementation for "Copy As PPMCK MML" is complete
- Fixed period/frequency values being written to several expansion chips when no note is active
- Fixed Qxy and Rxy being swapped on the noise channel
- Fixed N163 instrument editor not forcing the wave buffer to update
- Fixed Sxx effect on the DPCM channel resetting the 2A03 Pulse 1's period low byte
- Fixed Vxx in any N163 channel affecting other N163 channels (reverted to the Vxx behaviour in official builds)
- Fixed VRC7 detune table not being exported to NSFs



## V0.3.10 - 06/02/15

Additions:
- Added Exx effect for the FDS channel's hardware volume envelope control
- Added a hotkey to recall the current channel state on the status bar
- Added fixed tempo support, which ignores tempo and directly uses the speed value as the number of ticks per row

Improvements:
- Increased frame limit and pattern limit to 256
- Added menu commands to copy the current selection as volume sequence, text or PPMCK MML (from official 0.3.5)
- Added keyboard shortcut and menu entry for compact view mode
- Added keyboard shortcuts and menu entries for selection in various scopes
- Echo buffer command "^0" is now allowed
- Moved DPCM states to "Display register state"

Bug fixes:
- Application title is finally set to "0CC-FamiTracker"
- Fixed undo issues with paste command and drag/drop paste
- Fixed tracker initiating drag operations at an incorrect position if the selection is created by dragging from right top to left buttom or vice versa
- Fixed tracker crashing while copying the selection as a volume sequence if the selection begins at the first row of the track
- Fixed paste command not applying to all columns of the last channel in the pattern editor
- Fixed paste operations applying an incorrect selection if the clip data only contains effect commands
- Removed flicker in "Display register state"
- "Display register state" no longer shows information of non-existent N163 channels
- Tracker no longer retrieves channel states from non-existent channels
- Tracker no longer retrieves channel states from the current position before switching to a new song while playing
- Fixed 2A03 channels incorrectly retrieving states of hardware effects
- Tracker properly retrieves states of the correct channels when some channels are muted
- Tracker now blocks row insertion/deletion when edit mode is disabled
- Fixed reverse command crashing the tracker when a fx4 column is selected as the last column
- "Remove Unused Instruments" now respects instruments with disabled sequences
- Fixed frame editor incorrectly updating the value of the current pattern if the cursor is moved by the mouse before modifying the first digit
- Fixed detune dialog displaying FDS frequencies at the incorrect octave
- Fixed notes not producing sound after EE0 cancels the length counter until the high frequency byte is updated
- Fixed length counter not working in MMC5 channels
- Exx effects no longer reset the linear counter on the triangle channel
- Triangle channel's linear counter value now resets to 0x7F before playing
- Fixed T7y effect not working in the tracker
- Fixed Txy effect not applying transpose to the echo buffer when the note transposes downwards
- Fixed N163 channels inverting the 4xy vibrato pitch offset in the tracker
- Fixed the tracker exporting invalid pattern data when exporting the triangle channel's length counter effect
- Fixed E00 - E1F length counter effects not working in exported NSFs
- Fixed A00 improperly resetting the channel volume when the Mxy effect is not used in exported NSFs
- Fixed Sxx and Lxx effects being applied to future notes in exported NSFs
- ASM export no longer crashes 0CC-FamiTracker
- WAV and NSFe export now handle track durations properly when disjoint parts of the same frame are visited twice in a loop



## V0.3.9 - 04/12/15

Additions:
- Selection can now span across frame boundaries
- Added several special pasting modes
- Added an option in the configuration menu to retrieve the current state of all channels from previous rows
- Added a compact view mode which displays only the note fields in the pattern editor and hides all other columns

Improvements:
- Ported to the version of NSF driver used in official FamiTracker 0.4.6
- The configuration menu's general tab is re-organized
- Added an option in the configuration menu to allow pasting to continuously write to subsequent frames
- Added an option in the configuration menu to display rows skipped by global effects
- Added an option in the configuration menu to use the extra keys on the numpad as hexadecimal digits
- The colours of the current row in the pattern editor can now be configured
- The pattern editor's font size can now be set between 5 and 30 inclusive
- The pattern editor's rendering is slightly improved
- Added a cleanup command to populate all frames with unique pattern copies
- "Preview next/previous frames" now displays all frames above and below the current frame
- The pattern editor now highlights inapplicable effects in red
- Automatic scrolling in the pattern editor can now move diagonally
- Clicking previous or next frames moves the cursor to the row below the mouse cursor
- Ctrl+Clear key now deletes entire note
- Pasting now supports the Impulse Tracker edit style (any field on the note, instrument or volume column registers as non-empty)
- Interpolation now works on all pattern columns, as well as effects that take multiple parameters
- Added a stretch selection command which generalizes expanding and shrinking
- Added keyboard commands to scroll the values of current selection by 0x10 (data entry up/down coarse in ModPlug)
- The frequency displays below the register displays now use note values
- The tracker completely blocks all operations in the pattern editor that modify pattern data when the edit mode is disabled
- The N163 wave memory is now graphically displayed along the wave registers
- Sequence MML field now accepts "L" for loop point and "R" for release point (like NSDL)
- Sequence MML field now splits hexadecimal strings into signed bytes
- The N163 wave buffer effect is renamed to Zxx

Bug fixes:
- Program now displays properly on higher DPI settings
- Fixed the register display showing notes below octave 1 improperly
- Fixed the register display evaluating incorrect notes for the 2A03 channels when PAL is selected
- Fixed the register display drawing a bar for the DPCM channel even when no note is being played
- The frequency displays below the register displays now use the correct order of N163 channels
- Fixed "Remove unused DPCM samples" detecting sample usage incorrectly
- The pattern editor's cursor now moves by the correct number of steps across frames that have been shortened when using arrow keys or the mouse wheel
- The pattern editor always displays the current frame up to the row being played if follow mode is enabled, disregarding skip effects above the row
- Fixed incorrect behaviour of editing the instrument column when the ModPlug Tracker edit style is enabled
- Fixed a bug where decreasing the number of effect columns hides the cursor if it is on the effect column just removed
- Fixed full row selection selecting hidden effect columns of the last channel
- Fixed Shift + Mouse Wheel applying to an incorrect area after a selection is cancelled by clicking within the selection
- Fixed Numpad Plus key incorrectly incrementing the cursor's data field to blank data
- Pasting no longer applies to hidden effect columns and rows hidden by global effects when "Preview next/previous frames" is enabled
- Interpolation no longer works on effect columns if the beginning and ending effect commands do not match
- Transposing a selection no longer affects the echo buffer access notes
- Fixed "Expand selection" overwriting one row outside the selection if the number of selected rows is odd
- Expand, shrink, Backspace delete, and Replace instrument now apply only to columns within the selection
- Fixed Replace All crashing the tracker or not updating the pattern editor view after replacement
- Fixed a bug where Replace All searches the same row more than once
- Fixed effect query on the Find / Replace tab not behaving properly when the effect column scope is set to "All"
- Both the Find / Replace tab and the text importer now accept the N163 wave buffer effect
- Disabled FTM import loading tracks with a different number of N163 channels than the current module to incorrect channels when 5B or VRC7 is also used
- Fixed groove settings allowing negative speed values
- Fixed incompatible behaviour of Qxy and Rxy by realizing the portamento effects as instantiations of 3xx automatic portamento
- Fixed Qxy and Rxy not working in Sunsoft 5B in the tracker
- Echo buffer no longer converts the note cut to an invalid note
- Echo buffer no longer converts blank entries to invalid notes when they are modified by transposing effects
- Fixed 2A03 hardware sweep units not resetting properly for Hxy/Ixy effects
- Fixed FDS-only NSFs corrupting during export
- Fixed bankswitched multichip NSFs corrupting during export
- Fixed Txy not working in exported NSFs
- Fixed the tracker sometimes not exporting Mxy effects to NSFs
- Fixed the tracker exporting invalid pattern data when the N163 wave buffer effect has an out-of-bound parameter



## V0.3.8 - 02/16/15

Additions:
- Added Txy delayed transpose effect
- Added an option to export raw data of all non-empty rows as CSV
- Effect hints now appear after updating a pattern effect command
- Added a menu command to remove all unused DPCM samples and DPCM assignments

Improvements:
- Ported to official FamiTracker 0.4.6 and NSF driver 2.11
- "Display register state" supports Nintendo MMC5, Konami VRC7, and Sunsoft 5B
- "Display register state" shows human-readable channel information for all expansion chips
- "Display register state" skips non-existent N163 channels in the volume/ frequency display
- FDS channel now uses the same volume table as exported NSFs, but an option in the configuration menu is added to use the old table in the tracker
- Added an option in the configuration menu to cut sub-volume at volume 1 when Axy or 7xy is active on a channel
- Pressing the Repeat key moves the cursor down on all pattern fields, except when using the Modplug tracker edit style
- Added hotkey to duplicate the current non-empty pattern below the cursor to the first unused pattern of the channel
- FTM import now supports detune tables and groove tables
- Sequence MML field now accepts "'" single quotation mark for repeating terms
- The Sunsoft 5B's volume level can now be modified in the chip mixer
- MMC5 can now output sound when the period register is below 0x008
- The status bar now displays the current row and frame index in hexadecimal numbers when "Show row numbers in hex" is enabled
- Qxy and Rxy now affect the first entry of the echo buffer immediately

Bug fixes:
- Sunsoft 5B channels no longer desync upon playing
- Fixed a bug with the Lxx effect where no note release is issued if the non- release parts of the instrument sequences have not finished
- Fixed Mxy not resetting the channel volume to the previous one in the tracker if it is interrupted by an Axy effect that is cancelled with an A00 on the same row as the new note
- Fixed a bug where Yxx on the DPCM channel affects the N163 chip when there are fewer than 8 N163 channels
- Effect S7F works properly in exported NSFs
- Detune settings dialog no longer writes detune tables upon clicking "Cancel"
- Fixed FTM import not accepting Sunsoft 5B instruments
- Replacement query now handles expansion chip-specific effects correctly



## V0.3.7 - 01/18/15

Additions:
- Added Yxx wave buffer access for N163 channels
- Added Find / Replace tab

Improvements:
- Added information panel for the detune settings dialog
- Added hotkey configuration for the Volume Mask command
- Each N163 instrument can hold more than 512 samples in exported NSFs
- Added warning upon removing expansion chips from the current FTM
- Double-clicking the pattern editor does not create a selection if the step size is zero
- Both the effect type and parameter are remembered together when recalled later using the Repeat key

Bug fixes:
- Fixed a bug where the text importer could not load N163 instruments with a wave size of larger than 128
- Fixed Shift + Mouse Wheel sometimes not working when "Warp pattern values" is not checked in the configuration menu
- Fixed truncation error of the Tune button in the detune settings dialog



## V0.3.6 - 01/09/15

Additions:
- Added ad-hoc support for NSF export using multiple expansion chips
- Added delayed channel volume effect in FamiTracker

Improvements:
- Improved Shift + Mouse Wheel behaviour
- Shift + Mouse Wheel wraps values only when the corresponding configuration is checked (disabled by default)
- Instruments reset the 0xy effect's counter when it plays an arpeggio scheme
- "x" and "y" are case-insensitive in the arpeggio scheme MML input box

Bug fixes:
- When "Preview next/previous frame" is checked, 0CC-FamiTracker now moves to the previous channel properly using the up arrow key when the step size is larger than 1 and the channel contains a Bxx, Cxx, or Dxx command
- Fixed expansion chip-exclusive effects not being properly exported to NSFs
- Fixed Shift + Mouse Wheel sometimes editing multiple values despite having no pattern selection
- FDS detune table is now in correct pitch when automatically generated by the cent offset in the detune settings dialog
- Fixed expansion chip selector sometimes not removing pattern data in newly created channels when both the expansion chip combination and the number of N163 channels have been modified



## V0.3.5 - 01/01/15

Additions:
- Added echo buffer
- Added support for NSFe export
- Added delayed channel volume effect (implemented in ASM only)
- Added specs.txt

Improvements:
- Text importer and exporter now supports detune tables and grooves
- Text importer now allows retry / ignore upon encountering an unknown command
- Blank field dash and row number are properly aligned to the centre

Bug fixes:
- Fixed N163 NSF export not working
- Text importer uses the correct form of channel-exclusive effects sharing the same letter (H, I, J)
- Fixed the groove settings dialog sometimes resetting the song speed
- Fixed 0CC-FamiTracker not reading Oxx effects when the channel is muted
- Fixed exported NSFs freezing upon using groove 00 as the default groove
- Fixed configuration volume not affecting Sunsoft 5B channels
- 0CC-FamiTracker can now save and load Sunsoft 5B instruments



## V0.3.4 - 12/28/14

Additions:
- Added groove settings
- Added GROOVES block in FTM format

Improvements:
- Namco 163 can use unused registers for the waveform memory when there are fewer than 8 N163 channels (up to 240 samples if there is only one channel)
- 0CC-FamiTracker now skips sequence, instrument and DPCM blocks upon saving if these blocks contain no data

Bug fixes:
- Fixed note cut not working on Sunsoft 5B channels in exported NSFs
- Fixed Qxy and Rxy not working properly on the noise channel



## V0.3.3 - 12/25/14

Additions:
- Added Exx effect for hardware envelope and length counter control
- Added Sxx effect for 2A03 triangle channel linear counter
- Added two demo FTMs demonstrating the 2A03 hardware features

Improvements:
- 2A03 hardware sweep emulation is more accurate
- Sunsoft 5B channels are properly tuned
- Sunsoft 5B uses a subtractive volume table to handle its exponential volume
- Shift + Mouse Wheel can alter individual values of two-parameter effects
- Shift + Mouse Wheel can now wrap values upon overflowing

Bug fixes:
- Noise channel no longer halts in pitch slides and relative arpeggio sequences due to the channel notes' intrinsic octave and pitch
- Fixed the 2A03 hardware envelope's divider not being properly reloaded
- Shift + Mouse Wheel now works properly when selection spans across channels
- Fixed expansion chip selector not removing pattern data in newly created channels



## V0.3.2 - 12/20/14

Additions:
- Added DETUNETABLES block in FTM format
- Implemented detune settings dialog

Improvements:
- Default N163 pitch table is more accurate
- Default VRC7 pitch table is more accurate (this is already implemented in the official FamiTracker releases)
- Removed detune table for Sunsoft 5B (now identical to 2A03 / NTSC)

Bug Fixes:
- Fixed a bug where the Lxx parameter in exported NSFs is 1 larger than in FTM



## V0.3.1 - 12/16/14

Bug Fixes:
- Fixed Vxx not working on 5B channels
- Fixed 5B noise mode sequences not working in NSF export



## V0.3.0 - 12/15/14

Additions:
- Added full support for the Sunsoft 5B expansion chip
- Added expansion chip selector from ipi's build
- Added one demo FTM demonstrating the 0CC-FamiTracker implementation of the Sunsoft 5B chip
- Added one demo FTM demonstrating the Lxx effect

Improvements:
- Frame editor returns to the first channel after modifying the pattern index on the last channel
- NSFs can be exported in linear mode (they were forced in bankswitching mode since the first version)



## V0.2.4 - 12/11/14

Additions:
- Added one demo FTM demonstrating the tempo fix
- Added detune settings (implemented in V0.3.2)

Bug Fixes:
- Fixed tempo truncation error in FamiTracker
- Fixed Lxx not working on the DPCM channel



## V0.2.3 - 12/07/14

Additions:
- Sunsoft 5B envelope toggle is now incorporated into instrument duty sequences
- Vxx for the Sunsoft 5B channels, same effect as 5B duty sequence entry

Improvements:
- Status bar now shows the combination of expansion chips in multichip FTMs
- 0CC-FamiTracker now removes all unused sequences upon removing unused instruments
- Undo reverts the "file is modified" flag if the original state of the FTM can be restored

Bug Fixes:
- Fixed scrolling during drag-and-drop when there are a lot of channels
- 0CC-FamiTracker now uses the QWERTY key code constants
- 0CC-FamiTracker can now load FTMs using all six expansion chips
- 0CC-FamiTracker no longer considers instruments used with no notes unused during clean up
- FDS instruments no longer set the "file is modified" flag upon editing if they use frequency modulation
- N163 instruments no longer set the "file is modified" flag upon editing if there is more than one waveform



## V0.2.2 - 12/03/14

Additions:
- Ctrl+Shift+M now toggles Namco 163 multiplexer emulation, enabled by default
- Namco 163 waveforms can be up to 128 samples long

Improvements:
- Multichip FTMs load properly when there are fewer than 8 N163 channels
- Instrument editor always moves the pattern editor cursor to one of the instrument's channel(s) whenever the cursor is on a different chip
- The resource file now uses English (United States) as the language for all files instead of Swedish (Sweden)
- Better easter egg
- N163 waves automatically prevent reading from the non-wave registers

Bug Fixes:
- Fixed NSF export bug for FDS and N163 which use invalid pitch table pointers
- Fixed NSF export bug for VRC7 where VRC7 channels are two octaves lower
- Lxx effect no longer affects new notes, only existing notes



## V0.2.1 - 11/29/14

Additions:
- Added an easter egg (included in source code since V0.3.8)

Improvements:
- Arpeggio sequence graph editor initializes at the correct vertical position in Scheme mode

Bug Fixes:
- Fixed a bug where arpeggio schemes cannot be input using the MML field



## V0.2.0 - 11/28/14

Additions:
- Added the ability to read custom pitch tables
- Added delayed note release effect (Lxx)

Bug Fixes:
- Program now displays properly on higher DPI settings



## V0.1.1 - 09/14/14

Improvements:
- MML field now displays arpeggio schemes with "x" and "y" properly
- MML field accepts "+x" and "+y" as terms

Bug Fixes:
- Arpeggio sequence graph editor will not affect "x" and "y" in sequence terms
- Fixed a bug that treats negative numerals in front of terms incorrectly



## V0.1.0 - 09/12/14

Additions:
- Initial release
- Added arpeggio schemes
- 5 demo FTMs, one with permission from ipi



---

For enquiries mail to nicetas.c@gmail.com



---



# FamiTracker

Change log

Version 0.4.6 - February 4 2015

---

## Version 0.4.6

 New stuff:
  - Pattern rows with unspecified instruments will use the selected instrument

 Fixed bugs:
  - Fixed some rendering bugs
  - Fixed instrument clone command bug


## Version 0.4.5

 New stuff:
  - Improved VRC7 emulation, clipping will occur when audio is too loud
  - Improved accuracy of tempo calculation
  - Added multi-channel support to the insert command
  - Added song selector to the wave export dialog
  - Added drag and drop support to instrument list for instrument reordering
  - Added hex support to macro sequence strings and waves (x00 and $00)
  - Added option to display channel register values

 Fixed bugs:
  - Fixed a bug with the transpose command when used without an active selection
  - Fixed a portamento up/down effect bug when used on a silent channel
  - Fixed a bug with Sxx effect on DPCM affecting the pulse 1 channel when exported to NSF
  - Fixed a bug when using delayed notes on the last row of a pattern
  - Fixed a few pattern editor render bugs
  - Fixed MIDI note on/off priority problem


## Version 0.4.4

 Fixed bugs:
  - Fixed a problem with fixed arpeggio not playing the final note
  - Fixed muting / unmuting channels affecting all channels when preview full row was enabled
  - Fixed toolbar icons not being visible in Windows XP
  - Fixed a problem that caused the 64th sample not staying assigned when file is loaded
  - Fixed note slides clearing the slide effect after reaching target note
  - Fixed the Select All (Ctrl+A) command behaviour to be more similar to IT
  - Fixed skip command (Dxx) in NSF code when skipping to any other row than 0
  - Fixed NES export
  - Updated PAL DPCM period table


## Version 0.4.3

 New stuff:
  - Added names to the built in VRC7 patches
  - Added option to preview full row when editing notes
  - Added option to display flat notes instead of sharps
  - Added an audio level mixer
  - Removed MIDI import

 Fixed bugs:
  - Added a fix for TNS-HFC carts when DPCM bankswitching is used
  - Corrected VRC7 tuning table
  - Fixed DPCM instrument editor keyboard bug
  - Fixed NSF export when using sequences with hidden loop points
  - Fixed NSF code crash when using too many assigned DPCM samples
  - Fixed crash on startup when NSFplay is located in the same folder
  - Fixed a problem with N163 and FDS waves that appeared when playing a module
  - Fixed some DPCM sample editor bugs
  - Fixed duplicated paste bug in the frame editor
  - Fixed some noise pitch slide effect bugs
  - Fixed the tremolo effect command (7xx) on VRC7 channels
  - Fixed note slide effects on VRC7


## Version 0.4.2

 New stuff:
  - Added selection + drag & drop capability to the frame editor
  - NSF exporter will optimize N163 waves
  - Added initial delta counter setting to the DPCM instrument editor
  - Added a text exporter / importer (by rainwarrior)
  - Added a shortcut item for the duplicate patterns command
  - Added a volume mask option to pattern editor (edit menu)
  - Added module comments dialog
  - Adjusted filtering of FDS audio emulation

 Fixed bugs:
  - Fixed extra effect columns not being copied on shift+drag
  - Fixed a problem with shift+mouse wheel
  - Fixed a few other FTM file import problems
  - Fixed a problem with FDS wave previewing
  - Fixed a bankswitching bug when exporting multisong NSFs
  - Fixed VRC6 and MMC5 not being silenced by the Cxx command
  - Fixed some problems with the Dxx command


## Version 0.4.1

 New stuff:
  - Added an instrument files menu
  - Added an option to copy volume column to text (Shift+copy)
  - Changed default key for note cut to '1'

 Fixed bugs:
  - Fixed some problems with the FTM import feature
  - Fixed vibrato (4xx) export bug
  - Fixed volume slide (Axx) export bug
  - Fixed undo bug after pasting from clipboard
  - Fixed problem when loading instrument files that contains sequences of max size
  - Fixed some wave file export bugs


## Version 0.4.0

New stuff:
 - Added fixed and relative arpeggio modes
 - Added DPCM bank switching
 - Added Namco expansion sound
 - Added pattern expand/shrink options
 - Added pattern & instrument deep clone commands (by coda)
 - Added assembly source export option
 - Added some NSF export optimizations
 - Added indication of unsaved files
 - Added support for 24bit and 32bit samples to the DPCM importer
 - Added better DPCM import resampler (by Jarhmander)
 - Added an option to toggle between old & new speed/tempo split-point
 - Moved the home/end key behaviour to the impulse tracker mode
 - Removed PAL option when using expansion chips
 - Increased auto-scroll speed
 - BPM calculation depends on the row highlight settings
 - Added rainwarrior's new VRC7 patches

Fixed bugs:
 - Fixed the file creation date being overwritten when saving files
 - Fixed VRC6 sawtooth pitch bug
 - Fixed VRC6 instrument switch bug
 - Fixed FDS modulation bug (by rainwarrior)
 - Fixed FDS fine pitch setting in the tracker
 - Fixed FDS instrument release behaviour
 - Fixed export problem that occured when trying to play unassigned samples
 - Fixed VRC7 custom instrument bug in exported NSFs
 - Fixed tremolo (7xx) bug in exported NSFs
 - Fixed volume problem when using the note halt command on VRC6 and MMC5


## Version 0.3.7

New stuff:
 - Added MML string copy/paste to VRC7 instrument editor
 - Added VRC7 built-in patch display
 - Added MML string copy/paste to FDS waveform editor
 - Added pattern font size selector
 - Added single instance option
 - Added play option to FTM file type in file explorer
 - Added context sensitive help (F1)
 - Ctrl+select to do whole channel selections
 - New effects:
   * H = FDS modulation depth
   * I = FDS modulation speed, high part
   * J = FDS modulation speed, low part

Fixed bugs:
 - Shortcuts with removed keys are saved
 - Fixed a crash bug when frame preview is disabled
 - Fixed a sequence editor crash bug
 - Fixed another VRC6 release sequence bug
 - Fixed the PAL flag bug when loading PAL FTMs
 - Fixed a pitch effect (Pxx) bug in exported NSFs
 - Fixed a portamento effect (3xx) bug on noise channel in exported NSFs
 - Fixed MIDI sync clock receiver
 - Fixed a square channels emulation bug
 - Fixed a song editor bug
 - Fixed a problem when saving VRC6 instrument files
 - Fixed a small vibrato export bug


## Version 0.3.6b4

Fixed bugs:
 - Fixed a problem when loading multiple FTI files with DPCM samples
 - Fixed a problem that occured when using undo on patterns with Dxx/Bxx effects
 - Fixed VRC7 sweeps, auto portamento & fine pitch setting
 - Fixed auto-arpeggio issue in the tracker
 - Fixed the problem that caused release points to sometimes disappear in saved files
 - Fixed some synchronization issues that could cause a crash when switching expansion chip sound (and probably also in other cases)
 - Fixed some dpcm editor crash bugs
 - Fixed a bug that crashed the program when loading files with FDS instruments and FDS expansion disabled
 - Fixed a bug that crashed the program when repeatedly pressing F12
 - Fixed Sxx effect in exported files
 - Fixed Qxx/Rxx effect behvaiour on FDS in tracker
 - Fixed Rxx effect issue on exported files
 - Fixed Rxx/Qxx effect when triggering a new note before the slide is done in exported files
 - Corrected the tremolo effect on FDS
 - Adjusted VRC7 mixing volume according to some hardware measurements
 - Changed the VRC7 built-in patch settings to a more accurate set
 - Fixed FDS pitch clipping in exported files
 - Disabled FDS hardware envelope trigger on note offs in exported files
 - Fixed a crash bug when inserting/removing frames when max count is used
 - Fixed a crash bug that occurred sometimes when using select-all in a channel with all effect columns enabled
 - Fixed another crash bug that occurred sometimes when creating selections in patterns
 - Mixed paste does not overwrite non-empty rows anymore
 - Note cut/release works on IT-mode when previewing notes
 - Fixed a bug in the Yxx-effect when exported to NSF
 - Fixed a crash bug when inputting MIDI data on an invalid channel
 - Fixed VRC6 sequence bug when triggering the release part
 - Fixed a bug where DPCM would fail in exported NSFs after removing instruments
 - Inverted the Pxx command in the FDS channel
 - Fixed Vxx effect inconsistency when exporting NSFs
 - Fixed a crash bug when using a custom refresh rate with expansion chips

 - VRC7 volume is working
 - Channel mute/unmute works
 - Frequency limits on > sq2
 - Fixed DPCM load bug

New features:
 - Module importing
 - Added a shortcut option to show/hide control panel (in settings/shortcuts)
 - Added alt+left/right to move to left/right channel
 - Added block begin/end commands (Alt+B/E, available in IT-mode)

Fixed beta bugs: (ocurred only in earlier betas)
 - Loading/saving dialog in windows xp
 - Copy/paste over program instances
 - Fixed the exit crash problem
 - Fixed DPCM samples problem when importing modules
 - Fixed the pattern skip bug


---

#### Note from Dn-FT maintainer:

`changelog.txt` did not exist prior to FT 0.4.0 source. Due to this, versions 0.3.6 and below were not documented there. The following changelog text is derived from `changelog.htm`, in the HTMLHelp source.

---

## Version 0.3.6

 - Added support for module importing
 - Added a shortcut option to show/hide control panel (in settings/shortcuts)
 - Added alt+left/right to move to left/right channel
 - Added block begin/end commands (Alt+B/E, available in IT-mode)
 - Exporter plugin code updated
 - Fixed a problem when loading multiple FTI files with DPCM samples
 - Fixed a problem that occured when using undo on patterns with Dxx/Bxx effects
 - Fixed VRC7 sweeps, auto portamento & fine pitch setting
 - Adjusted VRC7 mixing volume after hardware measurements
 - Changed the VRC7 built-in patch settings to a more accurate set
 - Fixed auto-arpeggio issue in the tracker
 - Fixed the problem that caused release points to sometimes disappear in saved files
 - Fixed some synchronization issues that could cause a crash when switching expansion chip sound
 - Fixed some DPCM editor crash bugs
 - Fixed a bug that crashed the program when loading files with FDS instruments and FDS expansion disabled
 - Fixed a bug that crashed the program when repeatedly pressing F12
 - Fixed Sxx effect in exported files
 - Fixed Rxx effect issue on exported files
 - Fixed Vxx effect inconsistency when exporting NSFs
 - Fixed Qxx/Rxx effect behvaiour on FDS in tracker
 - Fixed Rxx/Qxx effect when triggering a new note before the slide is done in exported files
 - Fixed a bug in the Yxx-effect when exporting to NSF
 - Corrected the tremolo effect on FDS
 - Inverted the Pxx command in the FDS channel
 - Fixed FDS pitch clipping in exported files
 - Disabled FDS hardware envelope trigger for note offs when exporting to NSF
 - Fixed a crash bug when inserting/removing frames when max count is used
 - Fixed a crash bug that occurred sometimes when using select-all in a channel with all effect columns enabled
 - Mixed paste does not overwrite non-empty rows anymore
 - Note cut/release works on IT-mode when previewing notes
 - Fixed a crash bug when inputting MIDI data on an invalid channel
 - Fixed VRC6 sequence bug when triggering the release part
 - Fixed a bug where DPCM would fail in exported NSFs after removing instruments
 - Fixed a crash bug when using a custom refresh rate with expansion chips


## Version 0.3.5

 - Delete key deleted two rows on the bottom row, fixed
 - Entering an instrument column number moves to that instrument
 - Fixed a bug that caused files with 64 instruments to not load
 - Fixed export bug where instruments with empty sequences caused the file to not work
 - New effect Sxx, cut note after xx frames
 - New effect Xxx, DPCM retrigger, xx = delay in frames
 - Added a feature to remove unused instruments & patterns
 - Load/save instrument & DPCM paths bug on Windows Vista & 7 fixed
 - Beeps on Alt+[key] removed
 - Redraws screen on row highlight change
 - Fixed pattern display when frame preview is disabled
 - Fixed the tracker/nsf inconcistency when note delay and speed change was used on the same row
 - DPCM file preview added
 - CTRL+click in frame editor to queue next frame when playing
 - Added VRC7 & FDS expansion chips
 - Copy & paste in frame editor. Ctrl+C to copy a row and Ctrl+V to paste
 - Added note release command. Release-part of sequences are defined by a '/' in the sequence string.
 - Shortcut editor supports combinations of ctrl+alt+shift
 - Added a new vibrato mode that bends both up and down, old mode is still available as an option
 - Fixed a bug that caused corruption when using undo after pattern size change
 - Export plugin support is added
 - New commands in the shortcut editor:
 - play song from start (F5)
 - play and repeat pattern (F6)
 - play from cursor (F7)
 - stop (F8)
 - move to pattern (F2) / frame editor (F3)
 - replace instrument (Alt+S)
 - Frame (order) window now has an edit mode, double click to activate it. Keys:
 - 0-F: selects a pattern
 - Ctrl+Up/Down: moves selected frame up/down
 - Insert/Delete: inserts and deletes frames
 - Ctrl+C: copy current frame, Ctrl+V: paste frame into selected frame


## Version 0.3.0

 - MMC5 expansion sound support
 - Improved pattern editor (possible to select multiple channels, drag'n'drop, control+drag = copy, shift+drag = mix)
 - Ctrl+wheel = Transpose selected notes
 - Shift+wheel = Increase/decrease instrument, volume and effect values
 - Scroll lock = Toggle follow mode
 - Control+D = Duplicate frame
 - It's possible to edit directly in the frame editor by double clicking
 - Customizable keys for note cut, repeat and clear field
 - Added second highlight
 - Added a repeat action key
 - Added a second highlight option
 - Fixed a bug with the tremolo effect


## Version 0.2.9

 - New effects Q, R (pitch slide), A (volume slide) A (volume slide)
 - Fixed a tempo bug in the tracker
 - VRC6 support added
 - Improved the song editor
 - Added options to disable follow mode when playing and customizable row highlighting
 - Added clear patterns command
 - Wave file export
 - Added .NES file exporting
 - Improved hardware sweep commands in the tracker again (accurate NSF player is needed to get those right)
 - Fixed a bug where NSFs would fail if there was deleted instruments in the file
 - Fixed a problem with NSF bankswitching
 - Fixed some problems with note delay command in tracker and NSF


## Version 0.2.7

 - New NSF code, creates smaller NSF files (and use less 6502 CPU).
 - NSF bank switching is used automatically when needed.
 - New pattern effects: V, square duty setting/noise mode; Y, DPCM sample offset
 - Settings edit boxes are locked from editing, double click to unlock.
 - Fixed some bugs in the instrument editor to avoid crashes.
 - Added modplug keyboard style when the modplug setting is enabled.
 - Optimized screen updates, the tracker should use less CPU.
 - Currently selected instrument is highlighted in the list box.
 - Added an option to mask the instrument when inserting new notes.
 - Added a tempo box to the song settings.
 - Added multiple-tap-of-home-key functionality.
 - Added an option to select the instrument currently being played by the selected channel.
 - Fixed mixed pasting, instruments is now also pasted.
 - Fixed the instrument sequence too long crash bug.
 - Added buttons for next/prev song in the toolbar.
 - Auto-arpeggio works for PC keyboard input.
 - The commands speed, jump, skip and halt are now handled from muted channels.
 - Notes won't be retriggered when pushing two buttons at the same time and then releasing one.
 - Added a shortcut editor to the option dialog, note that the default shortcuts will still be shown in menus.
 - Page up/down jump lenght is customizable from configuration.
 - Impulse tracker edit option added.
 - The cursor will move with the same length as step length when moving it.
 - Fixed a bug that occured when moving the cursor when multiple effect tracks are visible.
 - Changed the pith sequence interpreter.
 - Volume of channels will be rounded of to 1 instead of 0.
 - DPCM delay works in the tracker.
 - Fixed a problem that caused 8-bit wave files to fail when imported to DPCM.
 - Fixed the crash problem when using samples too long. 
 - Added a value gradients option for the volume column.
 - Paste works when edit mode is off.
 - Changed the noise channel to display frequencies instead of useless notes.
 - Square hardware sweep is adjusted.
 - Scrolling with the mouse wheel in free cursor mode will scroll the view and not cursor
 - New keys
   * Numpad 0 - 9 = Select instrument 0 - 9
   * Ctrl + Numpad = Set step size
   * Alt + F9 = Mute channel
   * Alt + F10 = Solo channel
   * Numpad / = Decrease octave
   * Numpad * = Increase octave
   * Ctrl + up = Next instrument
   * Ctrl + Down = Previous instrument
   * Alt + T = Mask instrument column


---

#### Note from Dn-FT maintainer:

`changelog.htm` did not exist prior to FT 0.2.7 source. There seems to be no recorded changelog for versions 0.2.6 and below.
