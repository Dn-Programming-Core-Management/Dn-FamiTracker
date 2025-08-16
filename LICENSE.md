# Licenses

The application is distributed under the
[GPLv3+ license](https://www.gnu.org/licenses/gpl-3.0.en.html), or any later
version.

```_
Dn-FamiTracker - NES/Famicom sound tracker
Copyright (C) 2020-2025 D.P.C.M.
FamiTracker Copyright (C) 2005-2020 Jonathan Liss
0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/.
```

## Tracker source code and libraries

The tracker source code is distributed under the
[GPLv3+ license](https://www.gnu.org/licenses/gpl-3.0.en.html), or any later
version.

- FamiTracker
	- Copyright (C) 2005-2020 Jonathan Liss
	- Licensed under
	  [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html), or any
	  later version.
- 0CC-FamiTracker
	- Copyright (C) 2014-2018 HertzDevil
	- Licensed under
	  [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html), or any
	  later version.
	- Copyright (C) 2006 Brezza, 2012-2025 Brad Smith
	- Used under an [informal license](Source/APU/nsfplay/readme.txt).
- VRC7/OPLL sound emulator from emu2413 v1.5.9
	- Copyright (C) 2001-2019 Mitsutaka Okazaki
	- Licensed under the [MIT license](https://mit-license.org/).
- FDS and N163 sound emulator from Mesen
	- Copyright (C) 2014-2024 Sour
	- Licensed under
	  [GPLv3](https://www.gnu.org/licenses/old-licenses/gpl-3.0.en.html), or any
	  later version.
- Blip_buffer 0.4.1
	- Copyright (C) 2003-2006 Shay Green
	- Licensed under
	  [LGPLv2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
	- modified by nyanpasu64
- Free FFT and convolution (C++)
	- Copyright (C) 2017 Project Nayuki
	- Licensed under the
	  [MIT license](https://mit-license.org/).
- JSON for Modern C++
	- Copyright (C) 2013-2024 Niels Lohmann <https://nlohmann.me>
	- Licensed under the [MIT license](https://mit-license.org/).
- libsamplerate
	- Copyright (C) 2012-2016, Erik de Castro Lopo <erikd@mega-nerd.com>.
	  All rights reserved.
	- Licensed under the
	  [BSD-2-Clause
	  license](https://www.freebsd.org/copyright/freebsd-license/).

## NSFs, Exported music, modules, etc.

The licenses mentioned for the NSF driver and the tracker source do not apply to
exported media made with the program that does not contain executable code that
resembles any source file, such as exported .wavs, .txts, .jsons, or .csvs.

[jsr's comment on music created with FT.](http://forums.famitracker.com/viewtopic.php?f=4&t=122&p=741#p741)

However, NSFs/ROMs contain machine code that more or less corresponds to the NSF
driver source, and thus the software licenses may apply. More details can be
found [in the NSF source's license](./Source/drivers/asm/LICENSE.md).

This does not apply to exported assembly or binary music data, as they do not
include the NSF driver data.

## NSF driver source code

Dn-FT NSF driver changes are under GPL v2 due to 0CC-FT. More details can be
found [in the NSF source's license](./Source/drivers/asm/LICENSE.md).
