@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0"

set "NO_PAUSE="
set "PLATFORM_TOOLSET=%PLATFORM_TOOLSET%"
set "PREMAKE_ACTION=vs2022"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--toolset" (
    shift
    if "%~1"=="" (
        echo [Error] --toolset requires a value, e.g. v145 / v143
        goto fail
    )
    set "PLATFORM_TOOLSET=%~1"
) else (
    echo [Error] Unknown argument: %~1
    echo Usage: Setup.bat [--toolset v145] [--no-pause]
    goto fail
)
shift
goto parse_args
:args_done

echo ==========================================
echo    XMenu Project Generator
echo ==========================================

if not exist "tools\premake5.exe" (
    echo [Error] tools\premake5.exe not found!
    goto fail
)

for %%T in (XBaseBootstrap XBasePayloadEntry XBaseSA XBaseVC XBaseIII) do (
    if not exist "lib\%%T.lib" (
        echo [Error] Missing local XBase SDK library: lib\%%T.lib
        goto fail
    )
)
if not exist "include\XBase\XBase.h" (
    echo [Error] Missing local XBase SDK headers: include\XBase\XBase.h
    goto fail
)

call :resolve_toolchain

if not defined PLATFORM_TOOLSET set "PLATFORM_TOOLSET=!RESOLVED_TOOLSET!"
if not defined PLATFORM_TOOLSET set "PLATFORM_TOOLSET=v145"

echo Generating Visual Studio project files (premake action: !PREMAKE_ACTION!)...
if defined MSBUILD_EXE echo Using MSBuild: !MSBUILD_EXE!
if defined VS_INSTALL echo Using VS install: !VS_INSTALL!
echo Using PlatformToolset: !PLATFORM_TOOLSET!
if defined AVAILABLE_TOOLSETS echo Available toolsets: !AVAILABLE_TOOLSETS!
echo NOTE: Microsoft\VC\v180 is MSBuild targets path; C++ toolset is usually v145 on VS 18.
tools\premake5.exe !PREMAKE_ACTION!
if errorlevel 1 (
    echo [Error] Failed to generate project files.
    goto fail
)

if not exist "build\XMenu.sln" (
    echo [Error] Failed to generate project files.
    goto fail
)

if exist "tools\apply_platform_toolset.vbs" (
    cscript //nologo "tools\apply_platform_toolset.vbs" "!PLATFORM_TOOLSET!" "build"
    if errorlevel 1 (
        echo [Error] Failed to rewrite PlatformToolset in vcxproj files.
        goto fail
    )
    echo [Info] PlatformToolset rewritten to !PLATFORM_TOOLSET! in build\*.vcxproj
)

echo.
echo Project generation completed successfully.
echo You can now open "build\XMenu.sln" in Visual Studio.
echo Prefer building via Build.bat.
goto success

:resolve_toolchain
set "MSBUILD_EXE="
set "VS_INSTALL="
set "CL_EXE="
set "RESOLVED_TOOLSET="
set "AVAILABLE_TOOLSETS="

if defined VSINSTALLDIR (
    if exist "%VSINSTALLDIR%MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD_EXE=%VSINSTALLDIR%MSBuild\Current\Bin\MSBuild.exe"
        set "VS_INSTALL=%VSINSTALLDIR%"
        if "!VS_INSTALL:~-1!"=="\" set "VS_INSTALL=!VS_INSTALL:~0,-1!"
    )
)

if not defined MSBUILD_EXE (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do (
            if not defined MSBUILD_EXE set "MSBUILD_EXE=%%i"
        )
    )
)

if not defined MSBUILD_EXE (
    for %%R in (
        "D:\SoftWare\Microsoft Visual Studio"
        "%ProgramFiles%\Microsoft Visual Studio"
        "%ProgramFiles(x86)%\Microsoft Visual Studio"
    ) do (
        if not defined MSBUILD_EXE if exist "%%~R\" (
            for %%Y in (18 2026 2025 2022 2019) do (
                if not defined MSBUILD_EXE (
                    for %%E in (Community Professional Enterprise BuildTools Preview) do (
                        if not defined MSBUILD_EXE if exist "%%~R\%%Y\%%E\MSBuild\Current\Bin\MSBuild.exe" (
                            set "MSBUILD_EXE=%%~R\%%Y\%%E\MSBuild\Current\Bin\MSBuild.exe"
                            set "VS_INSTALL=%%~R\%%Y\%%E"
                        )
                    )
                )
            )
        )
    )
)

if not defined MSBUILD_EXE (
    for /f "delims=" %%i in ('where MSBuild.exe 2^>nul') do (
        if not defined MSBUILD_EXE set "MSBUILD_EXE=%%i"
    )
)

if not defined MSBUILD_EXE (
    set "RESOLVED_TOOLSET=v145"
    exit /b 0
)

if not defined VS_INSTALL (
    for %%I in ("!MSBUILD_EXE!") do set "MSBUILD_BIN=%%~dpI"
    for %%I in ("!MSBUILD_BIN!\..\..\..") do set "VS_INSTALL=%%~fI"
)

if defined VS_INSTALL (
    for /f "delims=" %%D in ('dir /b /ad /o-n "!VS_INSTALL!\VC\Tools\MSVC" 2^>nul') do (
        if not defined CL_EXE (
            if exist "!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\Hostx86\x86\cl.exe" (
                set "CL_EXE=!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\Hostx86\x86\cl.exe"
            ) else if exist "!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\HostX86\x86\cl.exe" (
                set "CL_EXE=!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\HostX86\x86\cl.exe"
            )
        )
    )
)

set "HAS_V145="
set "HAS_V143="
set "HAS_V142="
set "HAS_V141="
if defined VS_INSTALL (
    for /d %%V in ("!VS_INSTALL!\MSBuild\Microsoft\VC\v*") do (
        for /d %%T in ("%%~fV\Platforms\Win32\PlatformToolsets\v*") do (
            if exist "%%~fT\Toolset.props" (
                set "TS=%%~nxT"
                if /i "!TS!"=="v145" set "HAS_V145=1"
                if /i "!TS!"=="v143" set "HAS_V143=1"
                if /i "!TS!"=="v142" set "HAS_V142=1"
                if /i "!TS!"=="v141" set "HAS_V141=1"
                if defined AVAILABLE_TOOLSETS (
                    set "AVAILABLE_TOOLSETS=!AVAILABLE_TOOLSETS!,!TS!"
                ) else (
                    set "AVAILABLE_TOOLSETS=!TS!"
                )
            )
        )
    )
)

if defined HAS_V145 (
    set "RESOLVED_TOOLSET=v145"
) else if defined HAS_V143 (
    set "RESOLVED_TOOLSET=v143"
) else if defined HAS_V142 (
    set "RESOLVED_TOOLSET=v142"
) else if defined HAS_V141 (
    set "RESOLVED_TOOLSET=v141"
) else (
    set "RESOLVED_TOOLSET=v145"
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