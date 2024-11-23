# Version Update Checklist

- [ ] Create Version Tag
	- Format: `Dnx.y.z.r`
		- x: Major (API change)
		- y: Minor (backward compatible change)
		- z: Patch (backward compatible bugfix)
		- r: Revision (backward compatible quick fixes for typos)
	- Double check that this tag is higher than the previous release tag
	- Version tag must be appropriate to Semantic Versioning
- [ ] Update changelogs:
	- [CHANGELOG.md](CHANGELOG.md), which will update [../Dn-Help/hlp/changelog_shell.htm](changelog_shell.htm) upon build
- [ ] Edit every file with version info:
	- [Dn-FamiTracker.rc](../Dn-FamiTracker.rc)
	- [version.h](../version.h)
- [ ] **IMPORTANT**: update ConfigVersion.cpp
- [ ] Update copyright years:
	- [LICENSE.txt](../LICENSE.txt)
	- all source files with copyright info:

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

- [ ] Create and push Version Tag to the repo
	- There is a GitHub Actions script to build a drafted release upon a tag push
	- Tag must be on commit deemed appropriate for release
- [ ] Edit and publish the draft release page in GitHub
	- [ ] Triple check that the tag on the release matches the Version Tag
	- [ ] Double check the build artifacts to be accurate and working
- [ ] Notify everyone about the new version