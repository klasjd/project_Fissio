@echo off
chcp 65001 >nul
setlocal

:: Check administrator privileges and self-elevate if needed
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [Notice] Requesting administrator privileges...
    powershell -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

:: Resolve project root directory
set "SCRIPT_DIR=%~dp0"
set "PROJ_DIR=%SCRIPT_DIR%..\"
if exist "%SCRIPT_DIR%build\fissio-gui.exe" (
    set "PROJ_DIR=%SCRIPT_DIR%"
)

set "EXE_PATH=%PROJ_DIR%build\fissio-gui.exe"
set "WORK_DIR=%PROJ_DIR%build"
set "ICO_PATH=%PROJ_DIR%fissio_gui.ico"

if not exist "%EXE_PATH%" (
    echo [Error] Executable not found: %EXE_PATH%
    pause
    exit /b 1
)

echo ==============================================================
echo        Fissio Windows Startup Registration (Auto-Start)
echo ==============================================================
echo.

:: 1. Register with Windows Task Scheduler (Runs with highest privileges on logon in background)
echo [*] Registering in Task Scheduler (Highest privileges / tray auto-start)...
schtasks /create /tn "FissioAutoStart" /tr "\"%EXE_PATH%\" --tray" /sc onlogon /rl highest /f >nul 2>&1

if %errorlevel% equ 0 (
    echo [Success] Task Scheduler registration completed! (Starts silently without UAC prompt)
) else (
    echo [Warning] Task Scheduler registration failed.
)

:: 2. Register shortcut in Startup folder (Backup integration)
set "STARTUP_DIR=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
powershell -Command "$wsh = New-Object -ComObject WScript.Shell; $sc = $wsh.CreateShortcut('%STARTUP_DIR%\Fissio.lnk'); $sc.TargetPath = '%EXE_PATH%'; $sc.Arguments = '--tray'; $sc.WorkingDirectory = '%WORK_DIR%'; if (Test-Path '%ICO_PATH%') { $sc.IconLocation = '%ICO_PATH%,0' }; $sc.Save()" >nul 2>&1
echo [Success] Startup folder shortcut created!

echo.
echo ==============================================================
echo  Registration completed!
echo  Fissio will now automatically launch into the system tray on logon.
echo ==============================================================
echo.
pause
