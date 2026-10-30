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

echo ==============================================================
echo        Fissio Windows Startup Unregistration
echo ==============================================================
echo.

:: 1. Delete scheduled task
echo [*] Removing Fissio task from Task Scheduler...
schtasks /delete /tn "FissioAutoStart" /f >nul 2>&1

:: 2. Delete shortcut from Startup folder
set "STARTUP_DIR=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
if exist "%STARTUP_DIR%\Fissio.lnk" (
    del /f /q "%STARTUP_DIR%\Fissio.lnk" >nul 2>&1
)

echo [Success] Fissio auto-start registration successfully removed.
echo.
pause
