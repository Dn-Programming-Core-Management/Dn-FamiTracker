# Contribution Guide
Welcome! Thank you for considering to contribute to Dn-FamiTracker. We really need all the help we can get, since there aren't many people who help maintain FamiTracker and its forks. The following serves as a guide to those who want to get started with contributing.



## Code Guidelines and Formatting

The code mostly follows Unix style (LF) line endings and TAB indentation format. Remove trailing whitespace whenever you edit nearby lines.

There is not much documentation regarding the FamiTracker source code, so you might need to do some code reading first before you implement or modify a component. Document Dn-FamiTracker code behavior as much as possible whenever you are learning about them, this may be useful to other contributors.

FamiTracker (and most of its forks) is an MFC application with project files made in Visual Studio. Dn-FamiTracker supports being built by Visual Studio and CMake, so if you have implemented a change, be sure to update the CMake files as required.




## Dependencies and Building
To edit and/or build the source, you may use Visual Studio 2019, or alternatively, any IDE that supports CMake. You will need the following dependencies:

- HTMLHelp to build the manual (install from here: https://docs.microsoft.com/en-us/previous-versions/windows/desktop/htmlhelp/microsoft-html-help-downloads)
- For any IDE that supports building via CMake:
  - CMake
  - The latest MSVC build tools (may be from VS Installer or from other sources)
  - These dependencies can be installed through the Visual Studio Installer:
     - C++ MFC for latest v142 build tools (x86 & x64)
     - C++ ATL for latest v142 build tools (x86 & x64)

- For Visual Studio 2019:
   - C++ Windows XP Support for VS 2017
   - Windows Universal CRT SDK
   - C++ MFC for latest v142 build tools (x86 & x64)
   - C++ ATL for latest v142 build tools (x86 & x64)
   - The Desktop development with C++ workload, which includes:
      - MSVC v142 - VS 2019 C++ x64/x86 build tools (latest version)
      - C++ CMake tools for Windows

  - Alternatively, you can install the components mentioned via the provided .vsconfig file.



## Important Things to Note

- When committing changes, **file extension case must be the same as the original file!** This might result in merge conflicts, because Git is case sensitive, but in Windows systems, the file system is case insensitive by default.
- Additionally, case sensitivity in Windows can be enabled through WSL, but it **must only be enabled to resolve merge conflicts regarding file extension case sensitivity**. If case sensitivity is left enabled, Visual Studio throws a bunch of errors due to the way IntelliSense capitalizes paths internally.