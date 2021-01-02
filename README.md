<img src="docs/dn logo.svg">

Dn-Famitracker is a fork of 0CC-FamiTracker that incorporates numerous fixes and features. The meaning of the name "Dn" is whatever you wish it to be.

## Notable additions

- DPCM sample bit order reversal

- Adaptable register state refresh rate depending on playback (≈60fps on playback, otherwise 10fps)

- Multitrack per-channel export

- More accessible DPCM preview pitch in the instrument editor dialog

## Contributing

[Contributing and compiling](CONTRIBUTING.md)

## Downloads

Download releases: [![GitHub all releases](https://img.shields.io/github/downloads/Gumball2415/Dn-FamiTracker/total?logo=github&style=flat-square)](https://github.com/Gumball2415/Dn-FamiTracker/releases)

Development builds: [![AppVeyor](https://img.shields.io/appveyor/build/Gumball2415/dn-famitracker?logo=appveyor&style=flat-square)](https://ci.appveyor.com/project/Gumball2415/dn-famitracker/history)



The application and the source code are distributed under the GNU GPL 2 license. Dn modifications and additions in the source code and ASM code are marked with "// // !!" and ";; ;; !!", respectively.

The two readmes below are for archival purposes only.

# nyanpasu64 0CC-Famitracker

## Shutting down

0.6.3 is the final release. This program has been effectively dead for months to years, as my priorities have moved elsewhere, to building a new tracker from scratch, freed from MFC and being chained to Win32, freed from DirectSound and 40-70 ms of audio latency, freed from a fixed row grid that falls apart as soon as you try to use triplets...

Additionally I can't let this program continue under its current name. I can't have people talking to me about the program under its current name. I should've renamed the program earlier, but never picked one.

I invite the community to fork the program, possibly under a name like FamiTracker-Next. I may participate if I regain interest.

## Readme

- Download at https://github.com/nyanpasu64/j0CC-FamiTracker/releases
- Dev builds at https://ci.appveyor.com/project/nyanpasu64/0cc-famitracker/history

This is a fork of HertzDevil's 0CC-Famitracker 0.3.14.5 (since 0.3.15.1 and master are quite buggy and changing rapidly). It contains bugfixes which HertzDevil has not merged yet (some for months, some fixed independently in 0.3.15.1 or master), as well as N163 multi-wave copy-paste support.

Bugfixes:

- Export N163 FTI instruments properly.
- Don't corrupt memory when entering MML volume sequences over 252 items long (instead truncate).
- Fix bug where find-replacing anything with an empty effect creates " 00" effect.
- Fix text import instrument loop/release (@owomomo, fixed in 0.3.15.1).
- Update channel count after importing text (like master). Mark file as modified.

Enhancements:
- N163 file-specific mixing level offset.
- Typing Pxx (or FDS Zxx) defaults to P80.
- N163 wave editor's copy/paste buttons copy all waves at once, separated/terminated with semicolons. This allows for highly efficient Audacity-N163 import workflows (see https://gist.github.com/nyanpasu64/424110eab84dad50cf1a6646a72b2627).
- Save TXT export path properly.
- Hi-res FFT spectrogram (like master).

Future changes:

- Exponential instrument volume decay or release (like ADSR), and possibly an effect.
- Warn when editing instrument sequences reused in many instruments, or patterns appearing in many frames.
- https://github.com/nyanpasu64/j0CC-FamiTracker/issues

# 0CC-FamiTracker

0CC-FamiTracker is a modified version of FamiTracker that incorporates various bug fixes and new features which work in exported NSFs as well. The name "0CC" comes from the author's favourite arpeggio effect command. The current version includes:

- Partial FamiTracker 0.5.0 beta support
- Sound engine extensions:
   - Ad-hoc multichip NSF export
   - Echo buffer access
   - Polyphonic note preview
- New effects:
   - Hardware volume envelope effects
   - Delayed channel effects
   - FDS automatic FM effects
   - N163 wave buffer access effect
- Instrument extensions:
   - Arpeggio schemes
   - Instrument recorder
   - Compatible sequence instruments
- Interface extensions:
   - Find / replace tab
   - Transpose dialog
   - Split keyboard
- Extra module contents:
   - Detune settings
   - Groove settings
   - Bookmark manager
   - Linear pitch mode

See the change log for the full list of changes made in 0CC-FamiTracker.

This program and its source code are licensed under the GNU General Public License Version 2. Differences to the original FamiTracker source are marked with "// // //"; those to the ASM source with ";;; ;; ;" and "; ;; ;;;".

The current build is based on the version 0.5.0 beta 5 release of the official FamiTracker. 0CC-FamiTracker will be ported to newer official releases once they become available; features added in 0CC-FamiTracker may not have identical behaviour as the corresponding features on the official branch.

# Links

- http://hertzdevil.info/programs/  
  The download site for all versions of 0CC-Famitracker.
- http://0cc-famitracker.tumblr.com/  
  The official development log of 0CC-FamiTracker.
- http://hertzdevil.info/bug/main_page.php  
  The official bug tracker for all of HertzDevil's programs.
- http://github.com/HertzDevil/0CC-FamiTracker  
  The Git source repository for the tracker (this page).
- http://github.com/HertzDevil/0CC-FT-NSF-Driver  
  The Git source repository for the NSF driver.
