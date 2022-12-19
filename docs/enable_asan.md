# How to enable ASAN builds in Dn-FT:

1. Open in Visual Studio 2022.

2. Configure project properties as shown:

Project -> Dn-FamiTracker Properties
- Configuration Properties
	- General
		- Platform Toolset: Visual Studio 2022 (v143)
	- Advanced
		- Use of MFC: Use MFC in a Shared DLL
	- C/C++
		- General
			- Debug Information Format: Program Database (/Zi)
			- Enable Address Sanitizer: Yes (/fsanitize=address)
		- Code Generation
			- Runtime Library: set to DLL multi-threaded
				- on Debug builds, Multi-threaded Debug DLL (/MDd)
				- on Release builds, Multi-threaded DLL (/MD)
	 - Linker
		- General
			- Enable Incremental Linking: No (/INCREMENTAL:NO)
		- Input
			- Additional Dependencies
				- Use DLL MFC libraries instead of static MFC libraries
					- on Debug builds, replace `libcmtd.lib` and `nafxcwd.lib` with `MSVCRTD.lib`
					- on Release builds, replace `libcmt.lib` and `nafxcw.lib` with `MSVCRT.lib`
