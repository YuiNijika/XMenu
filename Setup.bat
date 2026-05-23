@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0"
if /i "%~1"=="--no-pause" set "NO_PAUSE=1"

echo ==========================================
echo    XMenu Project Generator
echo ==========================================

if not exist "tools\premake5.exe" (
    echo [Error] tools\premake5.exe not found!
    popd
    if not defined NO_PAUSE pause
    exit /b 1
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
    popd
    if not defined NO_PAUSE pause
    exit /b 1
)

echo Generating Visual Studio 2022 project files...
rem Generate VS 2022 project files for current Visual Studio versions.
tools\premake5.exe vs2022

echo.
if exist "build\XMenu.sln" (
    echo Project generation completed successfully.
    echo You can now open "build\XMenu.sln" in Visual Studio.
) else (
    echo [Error] Failed to generate project files.
)

popd
if not defined NO_PAUSE pause
