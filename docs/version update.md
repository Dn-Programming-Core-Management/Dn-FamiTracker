# How to update version info in Dn-FT:

- Update changelogs:
	- CHANGELOG.md, which will update upon build:
		- changelog.htm (in Dn-Help)
- Edit every file with version info:
	- Dn-FamiTracker.rc
	- Version.h
- IMPORTANT: update ConfigVersion.cpp
- Edit every file with the copyright info:
```
FamiTracker - NES/Famicom sound tracker
Copyright (C) 2005-2020 Jonathan Liss

0CC-FamiTracker is (C) 2014-2018 HertzDevil

Dn-FamiTracker is (C) 2020-2024 D.P.C.M.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Library General Public License for more details. To obtain a
copy of the GNU Library General Public License, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Any permitted reproduction of these routines, in whole or in part,
must bear this legend.
```
- Update copyright years in all licenses
	- LICENSE.txt
	- all source files with copyright info
- Push version tag to the repo
	- This triggers Github Actions to build a draft release
- Edit and publish the draft release page in Github