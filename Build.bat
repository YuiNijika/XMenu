@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0"

set "CONFIG=Release"
set "NO_PAUSE="
set "PLATFORM_TOOLSET=%PLATFORM_TOOLSET%"
set "PREMAKE_ACTION=vs2022"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="Debug" (
    set "CONFIG=Debug"
    shift
    goto parse_args
)
if /i "%~1"=="Release" (
    set "CONFIG=Release"
    shift
    goto parse_args
)
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
    shift
    goto parse_args
)
if /i "%~1"=="--toolset" goto parse_toolset
echo [Error] Unknown argument: %~1
echo Usage: Build.bat [Debug^|Release] [--toolset v145] [--no-pause]
goto fail

:parse_toolset
shift
if "%~1"=="" (
    echo [Error] --toolset requires a value, e.g. v145 / v143
    goto fail
)
set "TOOLSET_VALUE=%~1"
if /i "%TOOLSET_VALUE:~0,1%" NEQ "v" (
    echo [Error] Invalid platform toolset: %TOOLSET_VALUE%
    echo [Error] Expected a value such as v145 or v143.
    goto fail
)
for /f "delims=0123456789" %%C in ("%TOOLSET_VALUE:~1%") do (
    echo [Error] Invalid platform toolset: %TOOLSET_VALUE%
    echo [Error] Expected a value such as v145 or v143.
    goto fail
)
if "%TOOLSET_VALUE:~1%"=="" (
    echo [Error] Invalid platform toolset: %TOOLSET_VALUE%
    goto fail
)
set "PLATFORM_TOOLSET=%TOOLSET_VALUE%"
shift
goto parse_args

:args_done

echo ==========================================
echo    XMenu Builder
echo ==========================================
echo Configuration: %CONFIG%
echo Platform: Win32
echo.

if not exist "tools\premake5.exe" (
    echo [Error] tools\premake5.exe not found!
    goto fail
)

call :resolve_toolchain
if errorlevel 1 goto fail

if not defined PLATFORM_TOOLSET set "PLATFORM_TOOLSET=!RESOLVED_TOOLSET!"
if not defined PLATFORM_TOOLSET set "PLATFORM_TOOLSET=v145"
call :validate_platform_toolset "!PLATFORM_TOOLSET!"
if errorlevel 1 goto fail
call :ensure_toolset_available "!PLATFORM_TOOLSET!"
if errorlevel 1 goto fail
echo [Info] Resolved PlatformToolset: !PLATFORM_TOOLSET!
echo [Info] Available PlatformToolsets: !AVAILABLE_TOOLSETS!

rem XMenu consumes the staged XBase Release SDK only.
if /i not "%CONFIG%"=="Release" (
    echo [Error] Local XBase SDK currently contains Release libraries only.
    echo [Error] Build XBase Debug and stage it separately before using Debug.
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

if not exist "build" mkdir "build"

echo Using MSBuild: !MSBUILD_EXE!
if defined VS_INSTALL echo Using VS install: !VS_INSTALL!
if defined CL_EXE echo Using cl.exe: !CL_EXE!
echo Using PlatformToolset: !PLATFORM_TOOLSET!
if defined AVAILABLE_TOOLSETS echo Available toolsets: !AVAILABLE_TOOLSETS!
echo.
echo NOTE: Microsoft\VC\v180 is MSBuild targets path, NOT PlatformToolset.
echo       On VS 18 the real C++ toolset is usually v145.
echo.

echo Removing stale generated project files...
del /Q "build\*.sln" "build\*.vcxproj" "build\*.vcxproj.filters" "build\*.vcxproj.user" >nul 2>nul

echo Generating Visual Studio project files (premake action: !PREMAKE_ACTION!)...
tools\premake5.exe !PREMAKE_ACTION!
if errorlevel 1 (
    echo [Error] Project generation failed.
    goto fail
)

if not exist "build\XMenu.sln" (
    echo [Error] build\XMenu.sln not found after generation.
    goto fail
)

call :validate_xbase_sdk_boundary
if errorlevel 1 goto fail

rem Prefer MSBuild property override. File rewrite is optional (IDE convenience only)
rem and must never corrupt premake UTF-8 output.
call :apply_platform_toolset
if errorlevel 1 (
    echo [Warning] Could not rewrite vcxproj toolset; continuing with MSBuild /p:PlatformToolset.
) else (
    echo [Info] PlatformToolset rewritten to !PLATFORM_TOOLSET! in build\*.vcxproj
)

set "MSBUILD_PROPS=/p:Configuration=%CONFIG% /p:Platform=Win32 /p:PlatformToolset=!PLATFORM_TOOLSET! /verbosity:minimal"

echo Building loader build\bin\XMenu.asi...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenu !MSBUILD_PROPS!
if errorlevel 1 (
    echo.
    echo [Error] ASI loader build failed.
    goto fail
)

echo Building payload build\bin\XMenu\XMenuSA.dll...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuPayloadSA !MSBUILD_PROPS!
if errorlevel 1 (
    echo.
    echo [Error] SA payload build failed.
    goto fail
)

echo Building payload build\bin\XMenu\XMenuVC.dll...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuPayloadVC !MSBUILD_PROPS!
if errorlevel 1 (
    echo.
    echo [Error] VC payload build failed.
    goto fail
)

echo Building payload build\bin\XMenu\XMenuIII.dll...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuPayloadIII !MSBUILD_PROPS!
if errorlevel 1 (
    echo.
    echo [Error] III payload build failed.
    goto fail
)

echo Building installer build\bin\XMenuInstaller.exe...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuInstaller !MSBUILD_PROPS!
if errorlevel 1 (
    echo.
    echo [Error] Installer build failed.
    goto fail
)

if not exist "build\bin\XMenu.asi" (
    echo [Error] build\bin\XMenu.asi was not produced.
    goto fail
)
if not exist "build\bin\XMenu\XMenuSA.dll" (
    echo [Error] build\bin\XMenu\XMenuSA.dll was not produced.
    goto fail
)
if not exist "build\bin\XMenu\XMenuVC.dll" (
    echo [Error] build\bin\XMenu\XMenuVC.dll was not produced.
    goto fail
)
if not exist "build\bin\XMenu\XMenuIII.dll" (
    echo [Error] build\bin\XMenu\XMenuIII.dll was not produced.
    goto fail
)
if not exist "build\bin\XMenuInstaller.exe" (
    echo [Error] build\bin\XMenuInstaller.exe was not produced.
    goto fail
)

call :stage_data
if errorlevel 1 goto fail

echo.
echo Build completed successfully.
echo Output files:
echo   build\bin\XMenu.asi
echo   build\bin\XMenu\XMenuSA.dll
echo   build\bin\XMenu\XMenuVC.dll
echo   build\bin\XMenu\XMenuIII.dll
echo   build\bin\XMenuInstaller.exe
echo   build\bin\XMenu\data\{sa,vc,iii}\*.json
echo   build\bin\XMenu\data\i18n\{lang}\index.json
echo.
goto success

rem ============================================================
rem Generated projects must consume only the staged XBase SDK.
rem ============================================================
:validate_xbase_sdk_boundary
for %%P in (XMenu XMenuPayloadSA XMenuPayloadVC XMenuPayloadIII XMenuInstaller) do (
    if not exist "build\%%P.vcxproj" (
        echo [Error] Missing generated project: build\%%P.vcxproj
        exit /b 1
    )
)

rem Any generated target crossing into XBase sources or legacy SDKs is invalid.
findstr /S /I /M /C:"..\XBase" /C:"../XBase" /C:"PayloadEntry.cpp" /C:"BootstrapEntry.cpp" /C:"plugin-sdk" /C:"plugin.h" /C:"plugin_sa" /C:"plugin_vc" /C:"plugin_III" /C:"PSDK_DIR" /C:"PLUGIN_SDK_DIR" /C:"imgui" /C:"kiero" /C:"MinHook" /C:"MH_" "build\*.vcxproj" "build\*.sln" >nul 2>nul
if not errorlevel 1 (
    echo [Error] Generated projects crossed the staged XBase SDK boundary.
    exit /b 1
)

rem Installer owns its platform dependencies; loader and payloads must not.
findstr /I /M /C:"d3d9" /C:"Direct3D" /C:"urlmon.lib" /C:"Pdh.lib" /C:"shell32.lib" /C:"ole32.lib" "build\XMenu.vcxproj" "build\XMenuPayload*.vcxproj" >nul 2>nul
if not errorlevel 1 (
    echo [Error] Generated runtime projects still link platform or renderer libraries directly.
    exit /b 1
)

findstr /I /C:"XBaseBootstrap.lib" "build\XMenu.vcxproj" >nul 2>nul
if errorlevel 1 (
    echo [Error] Loader project does not link XBaseBootstrap.lib.
    exit /b 1
)
findstr /I /C:"/WHOLEARCHIVE:XBaseBootstrap.lib" "build\XMenu.vcxproj" >nul 2>nul
if errorlevel 1 (
    echo [Error] Loader project does not retain the XBase bootstrap entry object.
    exit /b 1
)
for %%G in (SA VC III) do (
    findstr /I /C:"XBase%%G.lib" "build\XMenuPayload%%G.vcxproj" >nul 2>nul
    if errorlevel 1 (
        echo [Error] Payload project %%G does not link XBase%%G.lib.
        exit /b 1
    )
    findstr /I /C:"XBasePayloadEntry.lib" "build\XMenuPayload%%G.vcxproj" >nul 2>nul
    if errorlevel 1 (
        echo [Error] Payload project %%G does not link XBasePayloadEntry.lib.
        exit /b 1
    )
    findstr /I /C:"/WHOLEARCHIVE:XBasePayloadEntry.lib" "build\XMenuPayload%%G.vcxproj" >nul 2>nul
    if errorlevel 1 (
        echo [Error] Payload project %%G does not retain the XBase payload entry object.
        exit /b 1
    )
)
exit /b 0

rem ============================================================
rem Pure-cmd toolchain resolve (no PowerShell).
rem PlatformToolset = folders under Platforms\Win32\PlatformToolsets
rem NOT the Microsoft\VC\v180 targets folder name.
rem ============================================================
:resolve_toolchain
set "MSBUILD_EXE="
set "VS_INSTALL="
set "CL_EXE="
set "RESOLVED_TOOLSET="
set "AVAILABLE_TOOLSETS="

rem 1) Developer Command Prompt already set VSINSTALLDIR
if defined VSINSTALLDIR (
    if exist "%VSINSTALLDIR%MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD_EXE=%VSINSTALLDIR%MSBuild\Current\Bin\MSBuild.exe"
        set "VS_INSTALL=%VSINSTALLDIR%"
        if "!VS_INSTALL:~-1!"=="\" set "VS_INSTALL=!VS_INSTALL:~0,-1!"
    )
)

rem 2) vswhere (optional)
if not defined MSBUILD_EXE (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do (
            if not defined MSBUILD_EXE set "MSBUILD_EXE=%%i"
        )
    )
)

rem 3) Hardcoded common roots (includes custom D:\SoftWare install)
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

rem 4) PATH fallback
if not defined MSBUILD_EXE (
    for /f "delims=" %%i in ('where MSBuild.exe 2^>nul') do (
        if not defined MSBUILD_EXE set "MSBUILD_EXE=%%i"
    )
)

if not defined MSBUILD_EXE (
    echo [Error] MSBuild.exe not found.
    echo Install Visual Studio with "Desktop development with C++".
    exit /b 1
)

rem Derive VS install from MSBuild path: ...\MSBuild\Current\Bin\MSBuild.exe
if not defined VS_INSTALL (
    for %%I in ("!MSBUILD_EXE!") do set "MSBUILD_BIN=%%~dpI"
    rem Bin\ -> Current\ -> MSBuild\ -> VS root
    for %%I in ("!MSBUILD_BIN!\..\..\..") do set "VS_INSTALL=%%~fI"
)

rem Find cl.exe under VC\Tools\MSVC
if defined VS_INSTALL (
    for /f "delims=" %%D in ('dir /b /ad /o-n "!VS_INSTALL!\VC\Tools\MSVC" 2^>nul') do (
        if not defined CL_EXE (
            if exist "!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\Hostx86\x86\cl.exe" (
                set "CL_EXE=!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\Hostx86\x86\cl.exe"
            ) else if exist "!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\HostX86\x86\cl.exe" (
                set "CL_EXE=!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\HostX86\x86\cl.exe"
            ) else if exist "!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\Hostx64\x86\cl.exe" (
                set "CL_EXE=!VS_INSTALL!\VC\Tools\MSVC\%%D\bin\Hostx64\x86\cl.exe"
            )
        )
    )
)

if not defined CL_EXE (
    echo [Error] cl.exe not found under: !VS_INSTALL!\VC\Tools\MSVC
    echo Open VS Installer -^> Modify -^> enable "Desktop development with C++" / MSVC x86/x64 build tools.
    exit /b 1
)

rem Collect real PlatformToolsets (prefer highest known id)
set "HAS_V145="
set "HAS_V143="
set "HAS_V142="
set "HAS_V141="
if defined VS_INSTALL (
    for /d %%V in ("!VS_INSTALL!\MSBuild\Microsoft\VC\v*") do (
        for /d %%T in ("%%~fV\Platforms\Win32\PlatformToolsets\v*") do (
            if exist "%%~fT\Toolset.props" (
                set "TS=%%~nxT"
                set "TOOLSET_AVAILABLE_!TS!=1"
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
    rem Last resort: VS18 defaults to v145 even if folder scan failed
    set "RESOLVED_TOOLSET=v145"
)

if not defined AVAILABLE_TOOLSETS set "AVAILABLE_TOOLSETS=!RESOLVED_TOOLSET!"
exit /b 0

:validate_platform_toolset
set "VALIDATED_TOOLSET=%~1"
if "!VALIDATED_TOOLSET!"=="" (
    echo [Error] PlatformToolset is empty.
    exit /b 1
)
if /i "!VALIDATED_TOOLSET:~0,1!" NEQ "v" (
    echo [Error] Invalid PlatformToolset: !VALIDATED_TOOLSET!
    echo [Error] Expected a value such as v145 or v143.
    exit /b 1
)
for /f "delims=0123456789" %%C in ("!VALIDATED_TOOLSET:~1!") do (
    echo [Error] Invalid PlatformToolset: !VALIDATED_TOOLSET!
    echo [Error] Expected a value such as v145 or v143.
    exit /b 1
)
if "!VALIDATED_TOOLSET:~1!"=="" (
    echo [Error] Invalid PlatformToolset: !VALIDATED_TOOLSET!
    exit /b 1
)
exit /b 0

:ensure_toolset_available
set "REQUESTED_TOOLSET=%~1"
call set "TOOLSET_FOUND=%%TOOLSET_AVAILABLE_%REQUESTED_TOOLSET%%%"
if not defined TOOLSET_FOUND (
    echo [Error] PlatformToolset !REQUESTED_TOOLSET! is not installed for Win32.
    echo [Error] Available toolsets: !AVAILABLE_TOOLSETS!
    exit /b 1
)
exit /b 0

rem ============================================================
rem Rewrite PlatformToolset in vcxproj without PowerShell.
rem ============================================================
:apply_platform_toolset
if not defined PLATFORM_TOOLSET exit /b 1
if not exist "build\*.vcxproj" (
    echo [Error] No vcxproj found under build\
    exit /b 1
)
if not exist "tools\apply_platform_toolset.vbs" (
    echo [Error] tools\apply_platform_toolset.vbs missing
    exit /b 1
)
cscript //nologo "tools\apply_platform_toolset.vbs" "!PLATFORM_TOOLSET!" "build"
if errorlevel 1 (
    echo [Error] Failed to rewrite PlatformToolset via cscript.
    exit /b 1
)
exit /b 0

:stage_data
if exist "tools\build_i18n_split.py" (
    py -3 "tools\build_i18n_split.py" >nul 2>nul
    if errorlevel 1 python "tools\build_i18n_split.py" >nul 2>nul
)

if exist "tools\generate_scene_visual_data.py" (
    py -3 "tools\generate_scene_visual_data.py" >nul 2>nul
    if errorlevel 1 python "tools\generate_scene_visual_data.py" >nul 2>nul
)

if exist "build\bin\XMenu\data" rmdir /S /Q "build\bin\XMenu\data"
if exist "build\bin\XMenu\i18n" rmdir /S /Q "build\bin\XMenu\i18n"
if not exist "build\bin\XMenu" mkdir "build\bin\XMenu"
if not exist "build\bin\XMenu\data" mkdir "build\bin\XMenu\data"

for %%G in (sa vc iii) do (
    if not exist "src\data\%%G" (
        echo [Error] Missing data directory: src\data\%%G
        exit /b 1
    )
    xcopy "src\data\%%G" "build\bin\XMenu\data\%%G\" /E /I /Y >nul
    if errorlevel 1 (
        echo [Error] Failed to stage data pack: %%G
        exit /b 1
    )
)

if not exist "build\bin\XMenu\data\i18n" mkdir "build\bin\XMenu\data\i18n"
for %%L in (zh en jp ru) do (
    if not exist "src\data\i18n\%%L\index.json" (
        echo [Error] Missing language index: src\data\i18n\%%L\index.json
        exit /b 1
    )
    xcopy "src\data\i18n\%%L" "build\bin\XMenu\data\i18n\%%L\" /E /I /Y >nul
    if errorlevel 1 (
        echo [Error] Failed to stage data language pack: %%L
        exit /b 1
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