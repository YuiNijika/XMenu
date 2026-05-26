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
echo    XMenu Builder
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

if not exist "build" mkdir "build"

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
echo Building loader build\bin\XMenu.asi...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenu /p:Configuration=%CONFIG% /p:Platform=Win32 /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [Error] ASI loader build failed.
    goto fail
)

echo Building payload build\bin\XMenu\XMenuSA.dll...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuPayloadSA /p:Configuration=%CONFIG% /p:Platform=Win32 /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [Error] SA payload build failed.
    goto fail
)

echo Building payload build\bin\XMenu\XMenuVC.dll...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuPayloadVC /p:Configuration=%CONFIG% /p:Platform=Win32 /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [Error] VC payload build failed.
    goto fail
)

echo Building payload build\bin\XMenu\XMenuIII.dll...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuPayloadIII /p:Configuration=%CONFIG% /p:Platform=Win32 /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [Error] III payload build failed.
    goto fail
)

echo Building installer build\bin\XMenuInstaller.exe...
"!MSBUILD_EXE!" "build\XMenu.sln" /m /t:XMenuInstaller /p:Configuration=%CONFIG% /p:Platform=Win32 /verbosity:minimal
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

call :stage_i18n
if errorlevel 1 goto fail

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
echo   build\bin\XMenu\i18n\{lang}\index.json
echo   build\bin\XMenu\data\{sa,vc,iii}\*.json
echo   build\bin\XMenu\data\i18n\{lang}\index.json
echo.
goto success

:stage_i18n
if exist "build\bin\XMenu\i18n" rmdir /S /Q "build\bin\XMenu\i18n"
if not exist "build\bin\XMenu" mkdir "build\bin\XMenu"
if not exist "build\bin\XMenu\i18n" mkdir "build\bin\XMenu\i18n"

if exist "tools\build_i18n_split.py" (
    py -3 "tools\build_i18n_split.py" >nul 2>nul
    if errorlevel 1 python "tools\build_i18n_split.py" >nul 2>nul
)

for %%L in (zh en jp ru) do (
    if not exist "src\data\i18n\%%L\index.json" (
        echo [Error] Missing language index: src\data\i18n\%%L\index.json
        exit /b 1
    )
    xcopy "src\data\i18n\%%L" "build\bin\XMenu\i18n\%%L\" /E /I /Y >nul
    if errorlevel 1 (
        echo [Error] Failed to stage language pack: %%L
        exit /b 1
    )
)
exit /b 0

:stage_data
if exist "tools\generate_scene_visual_data.py" (
    py -3 "tools\generate_scene_visual_data.py" >nul 2>nul
    if errorlevel 1 python "tools\generate_scene_visual_data.py" >nul 2>nul
)

if exist "build\bin\XMenu\data" rmdir /S /Q "build\bin\XMenu\data"
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