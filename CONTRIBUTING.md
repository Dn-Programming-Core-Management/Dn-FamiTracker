# Contribution Guide

Welcome! Thank you for considering to contribute to Dn-FamiTracker. We really need all the help we can get, since there aren't many people who help maintain FamiTracker and its forks. The following serves as a guide to those who want to get started with contributing.

---

## Dependencies and Building

To edit and/or build the source, you may use Visual Studio 2022, or alternatively, any IDE that supports CMake. You will need the following dependencies:

- [HTML Help Workshop](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/htmlhelp/microsoft-html-help-downloads) to build the manual.
	- Note that HTML Help Workshop is no longer supported and thus no longer available to download on Microsoft's website.
	- [Link to archived download.](https://web.archive.org/web/20200720082840/http://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe)
- [Pandoc](https://pandoc.org), for markdown document conversion.
	- Currently used for HTMLHelp manual changelog compiling.
- For NSF driver compiling:
	- [CC65 build tools](https://cc65.github.io/), make sure `ld65` and `ca65` is available in environment path.
- For miscellaneous custom build scripts:
	- [Python 3.10+](https://www.python.org/)
- For any IDE that supports building via CMake:
	- CMake version 3.16+
	- The latest MSVC build tools 
		- may be installed by VS Installer or by other sources
	- Windows 11 SDK (10.0.26100.0)
		- may be installed through VS Installer or by other sources
	- These dependencies can be installed through the Visual Studio Installer:
		- C++ MFC for latest v143 build tools (x86 & x64)
		- C++ ATL for latest v143 build tools (x86 & x64)
- For Visual Studio 2022:
	- Windows Universal CRT SDK
	- The **Desktop development with C++** workload, including:
		- MSVC v143 - VS 2022 C++ x64/x86 build tools (latest version)
		- C++ CMake tools for Windows
		- C++ AddressSanitizer
		- C++ ATL for latest v143 build tools (x86 & x64)
		- C++ MFC for latest v143 build tools (x86 & x64)
		- Windows 11 SDK (10.0.26100.0)
- Alternatively, you can install the components mentioned via the [provided .vsconfig file](../Dn-FT_VS_Dependencies.vsconfig).

---

## Code Guidelines and Formatting

- The code mostly follows Unix style (LF) line endings and TAB indentation format.
	- Remove trailing whitespace whenever you edit nearby lines.
- Document Dn-FamiTracker code behavior as much as possible whenever you are learning about them, this may be useful to other contributors.
	- There is not much documentation regarding the FamiTracker source code, so you might need to do some code reading first before you implement or modify a component.
- FamiTracker (and most of its forks) is an MFC application with project files made in Visual Studio. Dn-FamiTracker supports being built by Visual Studio and CMake, so if you have implemented a change, be sure to ***update the CMake files as required***.
- If implementing a new feature that affects tracker behavior (such as effects, UI), please ***update documentation*** in Dn-help and other areas of the tracker.
- If implementing a new feature that affects file formats (such as envelopes, new data format storage), ***please increment the respective file block version***.
	- if this necessitates a new file block, please increment the module file version.

---

## Git guidelines

- Write pull requests that passes CI builds and tests.
- Make sure to adhere to the [pull request template](Dn-FamiTracker/docs/pull_request_template.md) message guidelines.
- Base your pull request on the `main` branch.
- If a merge conflict happens due to not being updated in a long time, resolve merge conflicts and rebase your pull request to the latest compatible version of the `main` branch.
- Be sure to update [Dn-help](https://github.com/Dn-Programming-Core-Management/Dn-help) on your pull request as needed.
	- Create a corresponding pull request on Dn-help that links to your main pull request.
	- Then once the PR in Dn-help is merged, update the submodule to point to the main branch

---

## Important Things to Note

- When committing changes, ***file extension case must be the same as the original file!***
	- This might result in merge conflicts, because Git is case sensitive, but in Windows systems, the file system is case insensitive by default.
- Additionally, case sensitivity in Windows can be enabled through WSL, but it **must only be enabled to resolve merge conflicts regarding file extension case sensitivity**.
	- If case sensitivity is left enabled, Visual Studio throws a lot of errors due to the way IntelliSense capitalizes paths internally.

---

## For D.P.C.M. maintainers

- ***Do not push directly to the `main` branch***. Instead, push your changes to a branch first before writing a pull request.
- These branches are designated to be reoccurring may be used for the following:
	- `app-emu-module-nsf_driver-dev`
		- This branch is for modifying the application itself, such as the NSF driver, the module format, loading and saving code, emulator core, audio drivers, etc.
	- `docs-license-ver-meta-dev`
		- This branch is for meta related development, such as documentation, updating icons and logos, updating version info and license year text, etc.
	- `ci-dev`
		- This branch is for continuous integration development, such as GitHub Actions and AppVeyor.
		- You will most likely force push this branch to hell and back, so be sure to do it on your own fork repo.
- Other branches may be made for more niche/specific modifications and fixes.
- If your pull request touches two or more of these categories, it's fine but please keep it minimal.
	- Otherwise, create a new branch.
