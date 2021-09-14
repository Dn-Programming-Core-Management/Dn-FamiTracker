mkdir distribute
mkdir distribute\x64\Release
mkdir distribute\x64\Debug
mkdir distribute\x86\Release
mkdir distribute\x86\Debug

copy x64\Release\Dn-FamiTracker.exe	distribute\x64\Release\Dn-FamiTracker.exe
copy x64\Release\Dn-FamiTracker.chm	distribute\x64\Release\Dn-FamiTracker.chm
copy x64\Release\Dn-FamiTracker.pdb	distribute\x64\Release\Dn-FamiTracker.pdb
copy x64\Release\vc142.pdb 			distribute\x64\Release\vc142.pdb

copy x64\Debug\Dn-FamiTracker.exe	distribute\x64\Debug\Dn-FamiTracker.exe
copy x64\Debug\Dn-FamiTracker.chm	distribute\x64\Debug\Dn-FamiTracker.chm
copy x64\Debug\Dn-FamiTracker.pdb	distribute\x64\Debug\Dn-FamiTracker.pdb
copy x64\Debug\vc142.pdb			distribute\x64\Debug\vc142.pdb
copy changelog.txt					distribute\x64\changelog.txt

copy Win32\Release\Dn-FamiTracker.exe	distribute\x86\Release\Dn-FamiTracker.exe
copy Win32\Release\Dn-FamiTracker.chm	distribute\x86\Release\Dn-FamiTracker.chm
copy Win32\Release\Dn-FamiTracker.pdb	distribute\x86\Release\Dn-FamiTracker.pdb
copy Win32\Release\vc142.pdb			distribute\x86\Release\vc142.pdb

copy Win32\Debug\Dn-FamiTracker.exe		distribute\x86\Debug\Dn-FamiTracker.exe
copy Win32\Debug\Dn-FamiTracker.chm		distribute\x86\Debug\Dn-FamiTracker.chm
copy Win32\Debug\Dn-FamiTracker.pdb		distribute\x86\Debug\Dn-FamiTracker.pdb
copy Win32\Debug\vc142.pdb				distribute\x86\Debug\vc142.pdb
copy changelog.txt						distribute\x86\changelog.txt


@echo.
@echo distribute folder updated.
@echo.
@pause
