@echo off
setlocal EnableDelayedExpansion
rem Standalone builder for plugin-sdk static libs used by XMenu.
rem Usage: tools\build_plugin_sdk.bat [Release|Debug] [--toolset v145] [--no-pause]
pushd "%~dp0.."

set "CONFIG=Release"
set "NO_PAUSE="
set "PLATFORM_TOOLSET=%PLATFORM_TOOLSET%"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="Debug" (
    set "CONFIG=Debug"
) else if /i "%~1"=="Release" (
    set "CONFIG=Release"
) else if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--toolset" (
    shift
    if "%~1"=="" (
        echo [Error] --toolset requires a value
        goto fail
    )
    set "PLATFORM_TOOLSET=%~1"
) else (
    echo Usage: tools\build_plugin_sdk.bat [Release^|Debug] [--toolset v145] [--no-pause]
    goto fail
)
shift
goto parse_args
:args_done

if "%PLUGIN_SDK_DIR%"=="" (
    if exist "..\plugin-sdk\" (
        set "PLUGIN_SDK_DIR=%CD%\..\plugin-sdk"
    ) else if exist "E:\GTA\dev\plugin-sdk\" (
        set "PLUGIN_SDK_DIR=E:\GTA\dev\plugin-sdk"
    )
)
if not exist "%PLUGIN_SDK_DIR%" (
    echo [Error] PLUGIN_SDK_DIR not found. Set env var or place repo at ..\plugin-sdk
    goto fail
)
for %%I in ("%PLUGIN_SDK_DIR%") do set "PLUGIN_SDK_DIR=%%~fI"

set "MSBUILD_EXE="
set "VS_INSTALL="
for %%R in (
    "D:\SoftWare\Microsoft Visual Studio"
    "%ProgramFiles%\Microsoft Visual Studio"
    "%ProgramFiles(x86)%\Microsoft Visual Studio"
) do (
    if not defined MSBUILD_EXE if exist "%%~R\" (
        for %%Y in (18 2026 2025 2022) do (
            if not defined MSBUILD_EXE (
                for %%E in (Community Professional Enterprise BuildTools) do (
                    if not defined MSBUILD_EXE if exist "%%~R\%%Y\%%E\MSBuild\Current\Bin\MSBuild.exe" (
                        set "MSBUILD_EXE=%%~R\%%Y\%%E\MSBuild\Current\Bin\MSBuild.exe"
                        set "VS_INSTALL=%%~R\%%Y\%%E"
                    )
                )
            )
        )
    )
)

if not defined MSBUILD_EXE (
    echo [Error] MSBuild.exe not found
    goto fail
)
if not defined PLATFORM_TOOLSET set "PLATFORM_TOOLSET=v145"

if /i "%CONFIG%"=="Debug" (set "PSDK_CFG=zDebug") else (set "PSDK_CFG=Release")
set "PSDK_LIB=%PLUGIN_SDK_DIR%\output\lib"
if not exist "%PSDK_LIB%" mkdir "%PSDK_LIB%"

echo ==========================================
echo  Build plugin-sdk libs
echo ==========================================
echo PLUGIN_SDK_DIR: %PLUGIN_SDK_DIR%
echo Output:         %PSDK_LIB%
echo MSBuild:        %MSBUILD_EXE%
echo Toolset:        %PLATFORM_TOOLSET%
echo Config:         %PSDK_CFG%
echo.

set "PROPS=/p:Configuration=%PSDK_CFG% /p:Platform=Win32 /p:PlatformToolset=%PLATFORM_TOOLSET% /verbosity:minimal"

echo Building Plugin_SA...
"%MSBUILD_EXE%" "%PLUGIN_SDK_DIR%\plugin_sa\Plugin_SA.vcxproj" /m %PROPS%
if errorlevel 1 goto fail

echo Building Plugin_VC...
"%MSBUILD_EXE%" "%PLUGIN_SDK_DIR%\plugin_vc\Plugin_VC.vcxproj" /m %PROPS%
if errorlevel 1 goto fail

echo Building Plugin_III...
"%MSBUILD_EXE%" "%PLUGIN_SDK_DIR%\plugin_III\Plugin_III.vcxproj" /m %PROPS%
if errorlevel 1 goto fail

echo.
echo Done. Libraries:
dir /b "%PSDK_LIB%\*.lib"
popd
if not defined NO_PAUSE pause
exit /b 0

:fail
echo [Error] plugin-sdk build failed.
popd
if not defined NO_PAUSE pause
exit /b 1