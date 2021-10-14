@echo off
setlocal enabledelayedexpansion

cd /d %~dp0

if not exist "assets\compiled\" mkdir assets\compiled
for /f "usebackq delims=|" %%f in (`dir /b /a-d "assets\*.*"`) do (
	@REM set should_compile=0
	@REM if not exist "assets\compiled\%%f.o" (
	@REM	 set should_compile=1
	@REM ) else (
	@REM	 FOR /F "tokens=* USEBACKQ" %%g IN (`powershell -NoProfile -NonInteractive -c echo (^(^[DateTimeOffset]^(Get-Item "assets\compiled\%%f.o"^).LastWriteTime^).ToUnixTimeSeconds^(^) - ^(^[DateTimeOffset]^(Get-Item "assets\%%f"^).LastWriteTime^).ToUnixTimeSeconds^(^)^)`) do (SET "tdiff=%%g")
	@REM	 if !tdiff! lss 0 (
	@REM		 set should_compile=1
	@REM	 )
	@REM )

	@REM if "!should_compile!"=="1" (
		@REM echo Compiling assets\compiled\%%f.o...
		pushd assets
		ld -r -b binary -o "compiled\%%f.o" "%%f"
		popd
	@REM )
)

pushd res
windres res.rc res.o
popd

set CFLAGS=-Wall -Wextra -Werror -o Brokepad.exe  -Xlinker -Map=output2.map  ./brokepad.c -lwinmm -lgdi32 -lComdlg32 assets/compiled/*.o res/res.o

if "%1"=="prod" (
	gcc -Os -g0 -DNDEBUG -mwindows -Wl,--gc-sections,-u,main %CFLAGS%
	strip Brokepad.exe
) else (
	gcc -O0 -g3 %CFLAGS%
)
