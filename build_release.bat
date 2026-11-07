@echo off
setlocal
cd /d "%~dp0"

rem Visual Studio C++ environment (cl, link, ninja-compatible tools)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe not found. Install Visual Studio with the C++ workload.
    set "buildResult=1"
    goto :done
)
set "VSINSTALL="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (
    echo No Visual Studio installation with the C++ tools was found.
    set "buildResult=1"
    goto :done
)
call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
if errorlevel 1 (
    echo vcvarsall.bat x64 failed.
    set "buildResult=1"
    goto :done
)
rem The .NET AOT step (MUnique.Client.Library) calls vswhere, which vcvarsall does not put on PATH.
for %%d in ("%VSWHERE%") do set "PATH=%%~dpd;%PATH%"

if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\vcpkg"

cmake --preset windows-x64
if errorlevel 1 (
    set "buildResult=%errorlevel%"
    goto :done
)
cmake --build --preset windows-x64-release
set "buildResult=%errorlevel%"

:done
echo.
if "%buildResult%"=="0" (
    echo RELEASE BUILD SUCCESS
    echo Output: %~dp0out\build\windows-x64\src\Release\Main.exe
) else (
    echo RELEASE BUILD FAILED - exit code %buildResult%. See the error above.
)
echo.
pause
exit /b %buildResult%
