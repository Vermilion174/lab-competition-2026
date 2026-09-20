@echo off
CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
DOSKEY cl=cl /W4 /D _CRT_SECURE_NO_WARNINGS $*
set PATH=%PATH%;"C:\Program Files (x86)\TeraPad"
c:\windows\system32\cmd.exe