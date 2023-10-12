# How to manually enable ASAN builds in Dn-FT:

As of commit `f5d17004ec4c87912e3f5f16bac5c53bbc2b65ea`, ASAN builds are integrated within the solution. However, for earlier commits, this may be used as a guide.

1. Open in Visual Studio 2022.

2. Configure project properties as shown:

Project -> Dn-FamiTracker Properties
- Configuration Properties
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
