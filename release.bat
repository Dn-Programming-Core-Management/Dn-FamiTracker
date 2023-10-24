rem usage: release.bat configuration platform [commithash]

rem delete existing distribution files, if it exists
del distribute /q /f
mkdir distribute

if "%2"=="Win32" (
	goto continue
)
if "%2"=="x64" (
	goto continue
)
goto endfile

:continue
if "%~3"=="" goto compileversion

cd  %2/%1/
call 7z a -t7z -mx=9 -mmt=3 -m0=LZMA2:d=26:fb=128 -ms=on ..\..\distribute\Dn-FamiTracker_"%~3"_"%~2"_"%~1".7z Dn-FamiTracker.exe Dn-FamiTracker.chm Dn-FamiTracker.pdb vc143.pdb ..\..\docs\CHANGELOG.md ..\..\demo
cd ..\..
goto endfile

:compileversion
for /F "tokens=1,2,3,4,5 delims=, " %%A in (Dn-FamiTracker.rc) do (
	if "%%~A" == "FILEVERSION" (
		set version=v%%B%%C%%D%%E
	)
)
cd  %2/%1/
call 7z a -t7z -mx=9 -mmt=3 -m0=LZMA2:d=26:fb=128 -ms=on ..\..\distribute\Dn-FamiTracker_%version%_"%~2"_"%~1".7z Dn-FamiTracker.exe Dn-FamiTracker.chm Dn-FamiTracker.pdb vc143.pdb ..\..\docs\CHANGELOG.md ..\..\demo
cd ..\..
:endfile
