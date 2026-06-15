@echo off
REM ============================================================
REM   HypeClip AI - one-click installer for OBS Studio
REM   Just double-click this file. No admin password needed.
REM ============================================================
setlocal EnableDelayedExpansion
title HypeClip AI Installer

echo.
echo   ====================================================
echo      HypeClip AI  -  Never miss a moment.
echo      One-click installer for OBS Studio
echo   ====================================================
echo.

REM --- Where this .bat is sitting (the compiled files must be here too) ---
set "HERE=%~dp0"

REM --- Look for the compiled plugin next to this installer ---
set "DLL="
if exist "%HERE%hypeclip-ai.dll" set "DLL=%HERE%hypeclip-ai.dll"
if exist "%HERE%obs-plugins\64bit\hypeclip-ai.dll" set "DLL=%HERE%obs-plugins\64bit\hypeclip-ai.dll"

if "!DLL!"=="" (
  echo   [!] I could not find "hypeclip-ai.dll" next to this installer.
  echo.
  echo       This installer copies the plugin into OBS for you, but the
  echo       plugin file itself has to be built/compiled first.
  echo.
  echo       Put "hypeclip-ai.dll" ^(and the "data" folder^) in the SAME
  echo       folder as this installer, then run it again.
  echo.
  echo       See INSTALL-README.txt for how to get that file.
  echo.
  pause
  exit /b 1
)

REM --- Install into the per-user OBS plugin folder (no admin required) ---
set "TARGET=%APPDATA%\obs-studio\plugins\hypeclip-ai"
echo   Installing to:
echo     %TARGET%
echo.

mkdir "%TARGET%\bin\64bit"  >nul 2>&1
mkdir "%TARGET%\data"        >nul 2>&1

copy /Y "!DLL!" "%TARGET%\bin\64bit\hypeclip-ai.dll" >nul
if errorlevel 1 (
  echo   [!] Could not copy the plugin file. Is OBS open? Close it and retry.
  echo.
  pause
  exit /b 1
)

REM --- Copy the data folder (locale, profiles) if present ---
if exist "%HERE%data\" (
  xcopy /E /I /Y "%HERE%data\*" "%TARGET%\data\" >nul
)

echo   [OK] HypeClip AI is installed!
echo.
echo   Next steps:
echo     1. Close OBS Studio completely if it is open.
echo     2. Open OBS again.
echo     3. Top menu:  Docks  ^>  HypeClip AI   (tick it).
echo     4. That's it - it works automatically while you play.
echo.
pause
exit /b 0
