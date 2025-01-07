# Version Update Checklist

- [ ] Update changelogs:
	- [CHANGELOG.md](CHANGELOG.md), which will update [../Dn-Help/hlp/changelog_shell.htm](changelog_shell.htm) upon build
	- Format for changelog:
		- categories
			- `Important changes:`
				- summary of immediate and obvious changes to the program
			- `Improvements:`
				- additions and refactorings
			- `Bug fixes:`
				- any bug fixes
			- `Internal:`
				- meta changes, including changes regarding to the repository or CI
		- format
			- `<description of singular change> ([issue_author] [commit_author] [#<issue number>] [#<PR number>])`
			- only put in issue author and number if it exists
			- only put one author if issue and commit author is the same
			- sort by category, then by issue number, then by PR number
			- ex:
				- Fix access violation in MRU submenu list update (@eugene-s-nesdev @Gumball2415 #243 #214)
- [ ] Create Version Tag
	- Format: `Dnx.y.z.r`
		- x: Major (API change)
		- y: Minor (backward compatible change)
		- z: Patch (backward compatible bugfix)
		- r: Revision (backward compatible quick fixes for typos)
	- Double check that this tag is higher than the previous release tag
	- Version tag must be appropriate to Semantic Versioning
- [ ] Edit every file with version info:
	- [Dn-FamiTracker.rc](../Dn-FamiTracker.rc)
	- [version.h](../version.h)
- [ ] **IMPORTANT**: update ConfigVersion.cpp
- [ ] Update copyright years:
	- all source files with copyright info:

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

- [ ] Create and push Version Tag to the repo
	- There is a GitHub Actions script to build a drafted release upon a tag push
	- Tag must be on commit deemed appropriate for release
- [ ] Edit and publish the draft release page in GitHub
	- [ ] Triple check that the tag on the release matches the Version Tag
	- [ ] Double check the build artifacts to be accurate and working
- [ ] Notify everyone about the new version