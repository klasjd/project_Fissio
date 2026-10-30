@echo off
setlocal

set "PATH=C:\Program Files\Go\bin;C:\Program Files\Git\usr\bin;C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

cd /d "%~dp0"
set "ROOT_DIR=%~dp0"

if not exist build (
    mkdir build
)

if exist "build\fissio-gui.exe" (
    del /f /q "build\fissio-gui.exe.old" >nul 2>&1
    ren "build\fissio-gui.exe" "fissio-gui.exe.old" >nul 2>&1
)

if exist "build\fissio-wg.exe" (
    del /f /q "build\fissio-wg.exe.old" >nul 2>&1
    ren "build\fissio-wg.exe" "fissio-wg.exe.old" >nul 2>&1
)

cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

if not exist "build\wintun.dll" (
    if exist "third_party\wintun\wintun\bin\amd64\wintun.dll" (
        copy /y "third_party\wintun\wintun\bin\amd64\wintun.dll" "build\wintun.dll" >nul
    )
)

echo Building official WireGuard engine...
cd /d "%ROOT_DIR%third_party\fissio-wg"
go build -v -o "%ROOT_DIR%build\fissio-wg.exe"
cd /d "%ROOT_DIR%"
