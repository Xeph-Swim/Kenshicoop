@echo off
REM Build dist\sessiontest.exe - real NetLink/ENet loopback admission, roster,
REM star relay and disconnect-survivor integration test. Uses v100 x64.
setlocal

set "REPO=%~dp0.."
pushd "%REPO%" >nul
set "REPO=%CD%"
popd >nul

set "VS10=C:\Program Files (x86)\Microsoft Visual Studio 10.0"
set "VC=%VS10%\VC"
set "SDK=C:\Program Files\Microsoft SDKs\Windows\v7.1"
set "PATH=%VC%\bin\amd64;%VC%\bin;%VS10%\Common7\IDE;%SDK%\Bin\x64;%SDK%\Bin;%PATH%"
set "INCLUDE=%VC%\include;%SDK%\Include;%REPO%\third_party\vc10_compat;%REPO%\third_party\enet\enet\include;%REPO%\src\plugin"
set "LIB=%VC%\lib\amd64;%SDK%\Lib\x64"

if not exist "%REPO%\dist" mkdir "%REPO%\dist"
if not exist "%REPO%\build\sessiontest" mkdir "%REPO%\build\sessiontest"
set "ENET=%REPO%\third_party\enet\enet"

echo === Building sessiontest.exe (Release^|x64, v100) ===
where cl.exe
cl.exe /nologo /O2 /EHsc /W3 /DWIN32 /DWIN32_LEAN_AND_MEAN ^
    /Fo"%REPO%\build\sessiontest\\" ^
    /Fe"%REPO%\dist\sessiontest.exe" ^
    "%REPO%\src\sessiontest\main.cpp" ^
    "%REPO%\src\plugin\CoopLog.cpp" ^
    "%REPO%\src\plugin\net\NetLink.cpp" ^
    "%REPO%\src\plugin\net\SteamP2P.cpp" ^
    "%ENET%\callbacks.c" "%ENET%\compress.c" "%ENET%\host.c" "%ENET%\list.c" ^
    "%ENET%\packet.c" "%ENET%\peer.c" "%ENET%\protocol.c" "%ENET%\win32.c" ^
    ws2_32.lib winmm.lib
if errorlevel 1 (
    echo sessiontest build FAILED
    exit /b 1
)
echo sessiontest built: %REPO%\dist\sessiontest.exe
exit /b 0
