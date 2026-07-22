# Resolve a usable MSBuild + real C++ PlatformToolset.
# Prints KEY=VALUE lines for cmd consumption.
# Exit 0 on success, 1 on failure.
#
# Important:
#   Microsoft\VC\v180  is the MSBuild *targets folder*, NOT PlatformToolset.
#   Real PlatformToolset ids live under:
#     MSBuild\Microsoft\VC\v*\Platforms\Win32\PlatformToolsets\vXXX

$ErrorActionPreference = 'SilentlyContinue'

function Write-Kv([string]$k, [string]$v) {
    if ($null -ne $v -and "$v" -ne '') { Write-Output ("{0}={1}" -f $k, $v) }
}

function Get-VsWhere {
    foreach ($p in @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )) {
        if (Test-Path -LiteralPath $p) { return $p }
    }
    return $null
}

function Get-InstallCandidates {
    $list = New-Object System.Collections.Generic.List[string]

    if ($env:VSINSTALLDIR) {
        $p = $env:VSINSTALLDIR.TrimEnd('\')
        if (Test-Path -LiteralPath $p) { [void]$list.Add($p) }
    }

    $vswhere = Get-VsWhere
    if ($vswhere) {
        foreach ($req in @(
            'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
            'Microsoft.Component.MSBuild'
        )) {
            $paths = & $vswhere -products * -requires $req -property installationPath 2>$null
            foreach ($p in @($paths)) {
                if ($p -and (Test-Path -LiteralPath $p) -and -not $list.Contains($p)) {
                    [void]$list.Add($p)
                }
            }
        }
    }

    $roots = @(
        'D:\SoftWare\Microsoft Visual Studio',
        "$env:ProgramFiles\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    )
    $years = @('18', '2026', '2025', '2022', '2019')
    $editions = @('Community', 'Professional', 'Enterprise', 'BuildTools', 'Preview')
    foreach ($root in $roots) {
        if (-not (Test-Path -LiteralPath $root)) { continue }
        foreach ($y in $years) {
            foreach ($e in $editions) {
                $p = Join-Path $root (Join-Path $y $e)
                if ((Test-Path -LiteralPath $p) -and -not $list.Contains($p)) {
                    [void]$list.Add($p)
                }
            }
        }
    }
    return $list
}

function Get-ClExe([string]$vsInstall) {
    $msvcRoot = Join-Path $vsInstall 'VC\Tools\MSVC'
    if (-not (Test-Path -LiteralPath $msvcRoot)) { return $null }
    $versions = Get-ChildItem -LiteralPath $msvcRoot -Directory | Sort-Object Name -Descending
    foreach ($ver in $versions) {
        foreach ($rel in @(
            'bin\Hostx86\x86\cl.exe',
            'bin\HostX86\x86\cl.exe',
            'bin\Hostx64\x86\cl.exe',
            'bin\HostX64\x86\cl.exe',
            'bin\Hostx64\x64\cl.exe',
            'bin\HostX64\x64\cl.exe'
        )) {
            $cl = Join-Path $ver.FullName $rel
            if (Test-Path -LiteralPath $cl) { return $cl }
        }
    }
    return $null
}

function Get-PlatformToolsets([string]$vsInstall) {
    $found = New-Object System.Collections.Generic.List[string]
    $vcMsbuild = Join-Path $vsInstall 'MSBuild\Microsoft\VC'
    if (-not (Test-Path -LiteralPath $vcMsbuild)) { return $found }

    foreach ($vcVerDir in (Get-ChildItem -LiteralPath $vcMsbuild -Directory -Filter 'v*' -ErrorAction SilentlyContinue)) {
        foreach ($plat in @('Win32', 'x64')) {
            $ptsRoot = Join-Path $vcVerDir.FullName ("Platforms\{0}\PlatformToolsets" -f $plat)
            if (-not (Test-Path -LiteralPath $ptsRoot)) { continue }
            foreach ($ts in (Get-ChildItem -LiteralPath $ptsRoot -Directory -ErrorAction SilentlyContinue)) {
                $props = Join-Path $ts.FullName 'Toolset.props'
                if ((Test-Path -LiteralPath $props) -and -not $found.Contains($ts.Name)) {
                    [void]$found.Add($ts.Name)
                }
            }
        }
    }
    return $found
}

function Rank-Toolset([string]$name) {
    switch -Regex ($name) {
        '^v145' { return 145 }
        '^v143' { return 143 }
        '^v142' { return 142 }
        '^v141' { return 141 }
        '^v180' { return 5 }   # almost never a real PlatformToolset id
        '^v170' { return 4 }
        default {
            if ($name -match '^v(\d+)$') { return [int]$Matches[1] }
            return 0
        }
    }
}

$candidates = @()
foreach ($install in (Get-InstallCandidates)) {
    $msbuild = Join-Path $install 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path -LiteralPath $msbuild)) {
        $msbuild = Join-Path $install 'MSBuild\Current\Bin\amd64\MSBuild.exe'
    }
    if (-not (Test-Path -LiteralPath $msbuild)) { continue }

    $cl = Get-ClExe $install
    if (-not $cl) { continue }

    $toolsets = @(Get-PlatformToolsets $install)
    if ($toolsets.Count -eq 0) {
        # Last-resort guesses when props folders are incomplete.
        $toolsets = @('v145', 'v143', 'v142', 'v141')
    }

    foreach ($ts in $toolsets) {
        $candidates += [pscustomobject]@{
            Install  = $install
            MSBuild  = $msbuild
            Toolset  = $ts
            ClExe    = $cl
            Rank     = (Rank-Toolset $ts)
        }
    }
}

if (-not $candidates -or $candidates.Count -eq 0) {
    [Console]::Error.WriteLine('No usable Visual Studio C++ toolset found (need MSBuild + cl.exe + PlatformToolsets).')
    [Console]::Error.WriteLine('Install "Desktop development with C++" / MSVC x86/x64 build tools.')
    exit 1
}

$selected = $candidates |
    Sort-Object -Property @{Expression = 'Rank'; Descending = $true}, @{Expression = 'Install'; Descending = $true} |
    Select-Object -First 1

if ($env:REQUESTED_PLATFORM_TOOLSET) {
    $req = $env:REQUESTED_PLATFORM_TOOLSET.Trim()
    $hit = $candidates | Where-Object { $_.Toolset -eq $req } | Select-Object -First 1
    if ($hit) {
        $selected = $hit
    } else {
        [Console]::Error.WriteLine("Requested toolset '$req' not found. Available: $((($candidates | Select-Object -ExpandProperty Toolset -Unique) -join ', '))")
        exit 1
    }
}

$allTs = ($candidates | Select-Object -ExpandProperty Toolset -Unique | Sort-Object { - (Rank-Toolset $_) })

Write-Kv 'MSBUILD_EXE' $selected.MSBuild
Write-Kv 'PLATFORM_TOOLSET' $selected.Toolset
Write-Kv 'VS_INSTALL' $selected.Install
Write-Kv 'CL_EXE' $selected.ClExe
Write-Kv 'AVAILABLE_TOOLSETS' ($allTs -join ',')
exit 0