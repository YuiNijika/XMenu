@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0"

set "CONFIG=Release"
set "NO_PAUSE="

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="Debug" (
    set "CONFIG=Debug"
) else if /i "%~1"=="Release" (
    set "CONFIG=Release"
) else if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else (
    echo [Error] Unknown argument: %~1
    echo Usage: Build.bat [Debug^|Release] [--no-pause]
    goto fail
)
shift
goto parse_args
:args_done

echo ==========================================
echo    XMenu ASI Builder
echo ==========================================
echo Configuration: %CONFIG%
echo Platform: Win32
echo.

if not exist "tools\premake5.exe" (
    echo [Error] tools\premake5.exe not found!
    goto fail
)

rem Prefer the environment variable, then fall back to nearby plugin-sdk folders.
if "%PLUGIN_SDK_DIR%"=="" (
    if exist "..\plugin-sdk\" (
        set "PLUGIN_SDK_DIR=%~dp0..\plugin-sdk"
        echo [Info] Auto-detected PLUGIN_SDK_DIR=!PLUGIN_SDK_DIR!
    ) else if exist "..\..\plugin-sdk\" (
        set "PLUGIN_SDK_DIR=%~dp0..\..\plugin-sdk"
        echo [Info] Auto-detected PLUGIN_SDK_DIR=!PLUGIN_SDK_DIR!
    ) else (
        echo [Warning] PLUGIN_SDK_DIR environment variable is not set.
        echo Please enter the absolute path to your plugin-sdk directory.
        echo Example: E:\GTA\dev\plugin-sdk
        set /p PLUGIN_SDK_DIR="Path to plugin-sdk: "
    )
)

if not exist "%PLUGIN_SDK_DIR%" (
    echo [Error] Invalid plugin-sdk directory: %PLUGIN_SDK_DIR%
    goto fail
)

echo Generating Visual Studio 2022 project files...
tools\premake5.exe vs2022
if errorlevel 1 (
    echo [Error] Project generation failed.
    goto fail
)

if not exist "build\XMenu.sln" (
    echo [Error] build\XMenu.sln not found after generation.
    goto fail
)

call :find_msbuild
if not defined MSBUILD_EXE (
    echo [Error] MSBuild.exe not found.
    echo Please install Visual Studio 2022 with C++ build tools, or run this script from a Developer Command Prompt.
    goto fail
)

echo.
echo Using MSBuild: !MSBUILD_EXE!
echo Building build\XMenu.sln...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /p:Configuration=%CONFIG% /p:Platform=Win32 /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [Error] Build failed.
    goto fail
)

echo.
echo Build completed successfully.
echo Output directory: build\bin
goto success

:find_msbuild
set "MSBUILD_EXE="

if defined VSINSTALLDIR (
    if exist "%VSINSTALLDIR%MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD_EXE=%VSINSTALLDIR%MSBuild\Current\Bin\MSBuild.exe"
)

if not defined MSBUILD_EXE (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
            if not defined MSBUILD_EXE set "MSBUILD_EXE=%%i"
        )
    )
)

if not defined MSBUILD_EXE (
    for %%i in (
        "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    ) do (
        if exist %%~i if not defined MSBUILD_EXE set "MSBUILD_EXE=%%~i"
    )
)

if not defined MSBUILD_EXE (
    for /f "tokens=*" %%i in ('where MSBuild.exe 2^>nul') do (
        if not defined MSBUILD_EXE set "MSBUILD_EXE=%%i"
    )
)
exit /b 0

:success
popd
if not defined NO_PAUSE pause
exit /b 0

:fail
popd
if not defined NO_PAUSE pause
exit /b 1