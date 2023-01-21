rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
@echo off
echo // Generated Help Map file.  Used by Dn-FamiTracker.hhp. > "hlp\HTMLDefines.h"
echo. > "hlp\HTMLDefines.h"
echo // Commands (ID_* and IDM_*) >> "hlp\HTMLDefines.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "resource.h" >> "hlp\HTMLDefines.h"
echo. >> "hlp\HTMLDefines.h"
echo // Prompts (IDP_*) >> "hlp\HTMLDefines.h"
makehm /h IDP_,HIDP_,0x30000 "resource.h" >> "hlp\HTMLDefines.h"
echo. >> "hlp\HTMLDefines.h"
echo // Resources (IDR_*) >> "hlp\HTMLDefines.h"
makehm /h IDR_,HIDR_,0x20000 "resource.h" >> "hlp\HTMLDefines.h"
echo. >> "hlp\HTMLDefines.h"
echo // Dialogs (IDD_*) >> "hlp\HTMLDefines.h"
makehm /h IDD_,HIDD_,0x20000 "resource.h" >> "hlp\HTMLDefines.h"
echo. >> "hlp\HTMLDefines.h"
echo // Frame Controls (IDW_*) >> "hlp\HTMLDefines.h"
makehm /h /a "afxhh.h" IDW_,HIDW_,0x50000 "resource.h" >> "hlp\HTMLDefines.h"
start /wait hhc "hlp\Dn-FamiTracker.hhp"
if not exist "hlp\Dn-FamiTracker.chm" goto :HelpError
move "hlp\Dn-FamiTracker.chm" "Dn-FamiTracker.chm"
goto :HelpDone
:HelpError
echo hlp\Dn-FamiTracker.hhp(1) : error:Problem encountered creating help file
echo.
:HelpDone
echo.