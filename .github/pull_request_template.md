```
Pull request format:

This pull request aims to `<(specify reason)>`.

---

Changes:

- `<Describe change entry>`
	- Fixes/addresses issue `#<issue number>`.

Changelog format:
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

Notice:
- Please remove this code block and fill in the details.
- Be sure to update CHANGELOG.md in the `Unreleased` section, as well as the date last updated.
```
