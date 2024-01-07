# Contribution Guide

Welcome! Thank you for considering to contribute to Dn-FamiTracker. We really need all the help we can get, since there aren't many people who help maintain FamiTracker and its forks. The following serves as a guide to those who want to get started with contributing.

## Code Guidelines and Formatting

- The code mostly follows Unix style (LF) line endings and TAB indentation format.
	- Remove trailing whitespace whenever you edit nearby lines.
- There is not much documentation regarding the FamiTracker source code, so you might need to do some code reading first before you implement or modify a component.
- Document Dn-FamiTracker code behavior as much as possible whenever you are learning about them, this may be useful to other contributors.
- FamiTracker (and most of its forks) is an MFC application with project files made in Visual Studio. Dn-FamiTracker supports being built by Visual Studio and CMake, so if you have implemented a change, be sure to update the CMake files as required.

## Git guidelines

- Write pull requests that passes CI builds.
- Make sure to adhere to the [pull request template](pull_request_template.md) message guidelines.
- Base your pull request on the `main` branch.
- If a merge conflict happens due to not being updated in a long time, resolve merge conflicts and rebase your pull request to the latest reasonable version of the `main` branch.

### For Dn-FT maintainers:

- Do **NOT** push directly to the `main` branch. Instead, push your changes to these branches first before writing a pull request:
	- `app-emu-module-nsf_driver-dev`
		- This branch is for modifying the application itself, such as the NSF driver, the module format, loading and saving code, emulator core, audio drivers, etc.
	- `docs-license-ver-meta-dev`
		- This branch is for meta related development, such as documentation, updating icons and logos, updating version info and license year text, etc.
	- `ci-dev`
		- This branch is for continuous integration development, such as GitHub Actions and AppVeyor.
		- You will most likely force push this branch to hell and back, so be sure to do it on your own GitHub user's fork.
- Other branches may be made for more niche/specific modifications and fixes.
- If your pull request touches two or more of these categories, it's fine but please keep it minimal.
	- Otherwise, create a new branch as mentioned previously.

## Dependencies and Building

To edit and/or build the source, you may use Visual Studio 2022, or alternatively, any IDE that supports CMake. You will need the following dependencies:

- [HTML Help Workshop](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/htmlhelp/microsoft-html-help-downloads) to build the manual. Note that HTML Help Workshop is no longer supported and thus no longer available to download on Microsoft's website. [Link to archived download.](https://web.archive.org/web/20200720082840/http://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe)
- [Pandoc](https://pandoc.org), for markdown document conversion.
- For any IDE that supports building via CMake:
	- CMake
	- The latest MSVC build tools 
		- may be installed by VS Installer or by other sources
	- Windows 10 SDK version 2104 (10.0.20348.0)
		- may be installed by VS Installer or by other sources
	- These dependencies can be installed through the Visual Studio Installer:
		- C++ MFC for latest v143 build tools (x86 & x64)
		- C++ ATL for latest v143 build tools (x86 & x64)

- For Visual Studio 2022:
	- Windows Universal CRT SDK
	- The **Desktop development with C++** workload, including:
		- MSVC v142 - VS 2019 C++ x64/x86 build tools (latest version)
		- C++ CMake tools for Windows
		- C++ AddressSanitizer
		- C++ ATL for latest v143 build tools (x86 & x64)
		- C++ MFC for latest v143 build tools (x86 & x64)
		- Windows 10 SDK version 2104 (10.0.20348.0)

- Alternatively, you can install the components mentioned via the [provided .vsconfig file](../Dn-FT_VS_Dependencies.vsconfig).

## Important Things to Note

- When committing changes, **file extension case must be the same as the original file!** This might result in merge conflicts, because Git is case sensitive, but in Windows systems, the file system is case insensitive by default.
- Additionally, case sensitivity in Windows can be enabled through WSL, but it **must only be enabled to resolve merge conflicts regarding file extension case sensitivity**. If case sensitivity is left enabled, Visual Studio throws a bunch of errors due to the way IntelliSense capitalizes paths internally.
