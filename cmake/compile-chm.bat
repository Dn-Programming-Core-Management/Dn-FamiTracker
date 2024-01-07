rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
@echo off
powershell -Command "(Get-Content Dn-help\hlp\changelog_shell.htm).Replace('<!-- INSERT-CHANGELOG-HERE -->', (pandoc docs/CHANGELOG.md -f gfm -t html5)) | Set-Content -encoding UTF8 Dn-help\hlp\changelog.htm"
echo // Generated Help Map file.  Used by Dn-FamiTracker.hhp. > "Dn-help\hlp\HTMLDefines.h"
echo. > "Dn-help\hlp\HTMLDefines.h"
echo // Commands (ID_* and IDM_*) >> "Dn-help\hlp\HTMLDefines.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "resource.h" >> "Dn-help\hlp\HTMLDefines.h"
echo. >> "Dn-help\hlp\HTMLDefines.h"
echo // Prompts (IDP_*) >> "Dn-help\hlp\HTMLDefines.h"
makehm /h IDP_,HIDP_,0x30000 "resource.h" >> "Dn-help\hlp\HTMLDefines.h"
echo. >> "Dn-help\hlp\HTMLDefines.h"
echo // Resources (IDR_*) >> "Dn-help\hlp\HTMLDefines.h"
makehm /h IDR_,HIDR_,0x20000 "resource.h" >> "Dn-help\hlp\HTMLDefines.h"
echo. >> "Dn-help\hlp\HTMLDefines.h"
echo // Dialogs (IDD_*) >> "Dn-help\hlp\HTMLDefines.h"
makehm /h IDD_,HIDD_,0x20000 "resource.h" >> "Dn-help\hlp\HTMLDefines.h"
echo. >> "Dn-help\hlp\HTMLDefines.h"
echo // Frame Controls (IDW_*) >> "Dn-help\hlp\HTMLDefines.h"
makehm /h /a "afxhh.h" IDW_,HIDW_,0x50000 "resource.h" >> "Dn-help\hlp\HTMLDefines.h"
start /wait hhc "Dn-help\hlp\Dn-FamiTracker.hhp"
if not exist "Dn-help\hlp\Dn-FamiTracker.chm" goto :HelpError
move "Dn-help\hlp\Dn-FamiTracker.chm" "Dn-FamiTracker.chm"
goto :HelpDone
:HelpError
echo Dn-help\hlp\Dn-FamiTracker.hhp(1) : error:Problem encountered creating help file
echo.
:HelpDone
echo.