# Version Update Checklist

listing out things to do so i won't forget to do them -persune

- [ ] Update changelogs:
	- [CHANGELOG.md](Dn-FamiTracker/CHANGELOG.md), which will update [../Dn-Help/hlp/changelog_shell.htm](changelog_shell.htm) upon build
	- see [changelog format](changelog_format.md)
- [ ] Update copyright years:
	- Update License years in
		- [LICENSE.md](../LICENSE.md)
		- [README.md](../README.md)
	- Update all edited source files with latest copyright info:

```
Dn-FamiTracker - NES/Famicom sound tracker
Copyright (C) 2020-2026 D.P.C.M.
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

- [ ] ***important***: set date of release at the last moment!
- [ ] Create Version Tag
	- Format: `Dnx.y.z.b`
		- x: Major (API change)
		- y: Minor (backward compatible change)
		- z: Patch (backward compatible bugfix)
		- b: Build type (internal, not visible)
			- 0: stable release
			- 9: "dirty"/beta/in-development build
	- Double check that this tag is higher than the previous release tag
	- Version tag's first three digits must be appropriate to Semantic Versioning
- [ ] Edit every file with version info:`
	- [version.h](../version.h)
		- [Dn-FamiTracker.rc](../Dn-FamiTracker.rc) gets its version constants from here through a build script
	- [Readme.txt](../Readme.txt)
- [ ] **IMPORTANT**: update ConfigVersion.cpp
- [ ] [Dn-help update](../Dn-help/version_update.md)
- [ ] Create and push Version Tag to the repo
	- There is a GitHub Actions script to build a drafted release upon a tag push
	- Tag must be on commit deemed appropriate for release
- [ ] Edit and publish the draft release page in GitHub
	- [ ] Triple check that the tag on the release matches the Version Tag (apart from the build type number)
	- [ ] Double check the build artifacts to be accurate and working
- [ ] Notify everyone about the new version
