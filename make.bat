@echo off
setlocal enabledelayedexpansion

goto :cont

:error
echo error: program exited with code %errorlevel%
exit /b %errorlevel%


:cont
cd /d %~dp0

pushd res
windres res.rc res.o || goto :error
popd

set CFLAGS=-Wall -Wextra -Werror -o Brokepad.exe ./brokepad.c -lwinmm -lgdi32 -lComdlg32 -lshell32 res/res.o

if "%1"=="prod" (
	gcc -Os -g0 -flto -DNDEBUG -mwindows -Wl,--gc-sections,-u,main %CFLAGS% || goto :error
	strip Brokepad.exe || goto :error
) else if "%1"=="tcc" (
	tcc -g3 %CFLAGS% || goto :error
) else (
	gcc -O0 -g3 %CFLAGS% || goto :error
)
