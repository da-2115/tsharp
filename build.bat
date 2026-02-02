@echo off
REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

REM Run make
"C:\Program Files (x86)\GnuWin32\bin\make.exe" -f Makefile.win %*
