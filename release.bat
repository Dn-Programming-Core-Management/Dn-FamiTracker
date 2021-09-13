mkdir distribute
mkdir distribute\x64
mkdir distribute\x86

copy x64\Release\Dn-FamiTracker.exe distribute\x64\Dn-FamiTracker.exe
copy x64\Release\Dn-FamiTracker.chm distribute\x64\Dn-FamiTracker.chm
copy x64\Release\Dn-FamiTracker.pdb distribute\x64\Dn-FamiTracker.pdb
copy x64\Release\vc141.pdb distribute\x64\vc141.pdb
copy changelog.txt distribute\x64\changelog.txt
copy 0CC-readme.txt distribute\x64\0CC-readme.txt

copy Win32\Release\Dn-FamiTracker.exe distribute\x86\Dn-FamiTracker.exe
copy Win32\Release\Dn-FamiTracker.chm distribute\x86\Dn-FamiTracker.chm
copy Win32\Release\Dn-FamiTracker.pdb distribute\x86\Dn-FamiTracker.pdb
copy Win32\Release\vc141.pdb distribute\x86\vc141.pdb
copy changelog.txt distribute\x86\changelog.txt
copy 0CC-readme.txt distribute\x86\0CC-readme.txt


@echo.
@echo distribute folder updated.
@echo.
@pause
