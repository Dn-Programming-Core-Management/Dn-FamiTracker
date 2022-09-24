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
      -	Enable Address Sanitizer: Yes (/fsanitize=address)
	  - Code Generation
			-	Runtime Library: Multi-threaded Debug DLL (/MDd)
  - Linker
    - General
      - Enable Incremental Linking: No (/INCREMENTAL:NO)
