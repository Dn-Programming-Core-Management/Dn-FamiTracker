# Version Update Checklist

- [ ] Update changelogs:
	- [CHANGELOG.md](Dn-FamiTracker/CHANGELOG.md), which will update [../Dn-Help/hlp/changelog_shell.htm](changelog_shell.htm) upon build
	- Categories
		- `Important changes:`
			- summary of immediate and obvious changes to the program
		- `Improvements:`
			- additions and refactoring
		- `Bug fixes:`
			- any bug fixes
		- `Internal:`
			- meta changes, including changes regarding to the repository or CI
	- Format
		- `<description of singular change> ([issue_author] [commit_author] [#<issue number>] [#<PR number>])`
		- only put in issue author and number if it exists
		- only put one author if issue and commit author is the same
		- sort by category, then by issue number, then by PR number
		- ex:
			- `Fix access violation in MRU submenu list update (@eugene-s-nesdev @Gumball2415 #243 #214)`
- [ ] Create Version Tag
	- Format: `Dnx.y.z.b`
		- x: Major (API change)
		- y: Minor (backward compatible change)
		- z: Patch (backward compatible bugfix)
		- b: Build type (internal, not visible)
			- 0: stable release
			- 9: "dirty"/beta/in-development build
	- Double check that this tag is higher than the previous release tag
	- Version tag must be appropriate to Semantic Versioning
- [ ] Edit every file with version info:`
	- [version.h](../version.h)
		- [Dn-FamiTracker.rc](../Dn-FamiTracker.rc) gets its version constants from here
	- [Readme.txt](../Readme.txt)
- [ ] **IMPORTANT**: update ConfigVersion.cpp
- [ ] Update copyright years:
	- Update License years in
		- [LICENSE.md](../LICENSE.md)
		- [README.md](../README.md)
	- Update all source files with copyright info:

```
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

- [ ] ***important***: set date of release at the last moment!
- [ ] Create and push Version Tag to the repo
	- There is a GitHub Actions script to build a drafted release upon a tag push
	- Tag must be on commit deemed appropriate for release
- [ ] Edit and publish the draft release page in GitHub
	- [ ] Triple check that the tag on the release matches the Version Tag (apart from the build type number)
	- [ ] Double check the build artifacts to be accurate and working
- [ ] Notify everyone about the new version