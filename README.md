![Dn-FamiTracker banner logotype](docs/dn_logo.svg)

Dn-FamiTracker is a fork of 0CC-FamiTracker that incorporates numerous fixes and features.

The meaning of the name "Dn" is "Derivative n", which alludes to this fork being the nth derivative of the original FamiTracker program.

---

## Notable additions

- New effects: `Nxx`, `=xx`, `Kxx`
- Support for OPLL-as-VRC7
- NSF 2.0 and NSFe export support
- Fixed metadata support on NSF 2.0 and NSFe export
- Complete text import/export
- DPCM sample bit order reversal
- Multitrack per-channel export
- More accessible DPCM pitch preview
- More accessible VRC7 patch and envelope editing
- Improved FDS, N163 VRC7 and 2A03 emulation
- Restored Help manual, now under maintenance at [Dn-help](https://github.com/Dn-Programming-Core-Management/Dn-help)
- and more to come, see the [release page](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/releases) and the [changelog](docs/CHANGELOG.md) for more info

## Downloads

- Download releases: [![GitHub all releases](https://img.shields.io/github/downloads/Dn-Programming-Core-Management/Dn-FamiTracker/total?logo=github&style=flat-square)](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/releases)
- GitHub Actions automated builds: [![GitHub Actions](https://img.shields.io/github/actions/workflow/status/Dn-Programming-Core-Management/Dn-FamiTracker/build-artifact.yml?style=flat-square)](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/actions/workflows/build-artifact.yml)
- Github Actions automated release builds: [![GitHub](https://img.shields.io/github/actions/workflow/status/Dn-Programming-Core-Management/Dn-FamiTracker/build-release-artifact.yml?style=flat-square)](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/actions/workflows/build-release-artifact.yml)
- Legacy AppVeyor Gumball2415 builds: [![AppVeyor](https://img.shields.io/appveyor/build/Gumball2415/dn-famitracker-legacy?logo=appveyor&style=flat-square)](https://ci.appveyor.com/project/Gumball2415/dn-famitracker-legacy/history)
- Legacy AppVeyor automated D.P.C.M. builds: [![AppVeyor](https://img.shields.io/appveyor/build/Gumball2415/dn-famitracker?logo=appveyor&style=flat-square)](https://ci.appveyor.com/project/Gumball2415/dn-famitracker/history)
- AppVeyor notices:
	- Due to delayed AppVeyor reintegration, builds for commits `dc4c9e86` to `21413603` are not available.
	- Due to delayed AppVeyor branch reconfiguration, builds for commits `bc46c86c` to `a591d154` are not available.
	- Due to less reliability, AppVeyor builds beyond `2c997736` are not available.

## Licenses

The application and the source code are distributed under the [GPLv3+ license](https://www.gnu.org/licenses/gpl-3.0.en.html).

Copyright (C) 2020-2024 D.P.C.M.

### Tracker software and libraries

- FamiTracker
	- Copyright (C) 2005-2020 Jonathan Liss
	- Licensed under [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).
- 0CC-FamiTracker
	- Copyright (C) 2014-2018 HertzDevil
	- Licensed under [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).
- 2A03 sound emulator from NSFPlay
	- Copyright (C) 2006 Brezza, 2012-2025 Brad Smith
	- Used under an [informal license](Source/APU/nsfplay/readme.txt).
- VRC7/OPLL sound emulator from emu2413 v1.5.9
	- Copyright (C) 2001-2019 Mitsutaka Okazaki
	- Licensed under the [MIT license](https://mit-license.org/).
- FDS and N163 sound emulator from Mesen
	- Copyright (C) 2014-2024 Sour
	- Licensed under [GPLv3](https://www.gnu.org/licenses/old-licenses/gpl-3.0.en.html).
- Blip_buffer 0.4.1
	- Copyright (C) 2003-2006 Shay Green
	- Licensed under [LGPLv2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
	- modified by nyanpasu64
- Free FFT and convolution (C++)
	- Copyright (C) 2017 Project Nayuki
	- Licensed under the [MIT license](https://mit-license.org/).
- JSON for Modern C++
	- Copyright (C) 2013-2024 Niels Lohmann \<https://nlohmann.me>
	- Licensed under the [MIT license](https://mit-license.org/).
- libsamplerate
	- Copyright (C) 2012-2016, Erik de Castro Lopo \<erikd@mega-nerd.com>. All rights reserved.
	- Licensed under the [BSD-2-Clause license](https://www.freebsd.org/copyright/freebsd-license/).

### NSF Driver

On the FamiTracker website, the NSF driver code is distributed separately from the rest of the source code. This is because jsr wanted to license the driver under a more permissive license than GPL 2, but didn't seem to get around doing so.

HertzDevil erroneously licensed the modified 0CC-FamiTracker NSF driver code under GPL v2, which restricts anyone sharing compiled NSFs and binaries without sharing the assembly source.

Therefore, Dn-FT NSF driver changes are distributed under a separate MIT-0 license, while 0CC FT NSF driver changes are distributed under GPL v2.

The remaining original FT driver code is not under any license.

See [the license file](Source/drivers/asm/LICENSE) for more information.

## Contributing

[Contributing and compiling](docs/CONTRIBUTING.md)

## See also:

- nyanpasu64 0CC-FamiTracker (archived): https://github.com/nyanpasu64/j0CC-FamiTracker/
- 0CC-FamiTracker: https://github.com/HertzDevil/0CC-FamiTracker/
- FamiTracker: https://famitracker.com/
