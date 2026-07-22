# Rewrite <PlatformToolset> in build\*.vcxproj
param(
    [Parameter(Mandatory = $true)][string]$Toolset,
    [string]$BuildDir = 'build'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $BuildDir)) {
    throw "Build directory not found: $BuildDir"
}

$files = Get-ChildItem -LiteralPath $BuildDir -Filter '*.vcxproj' -File
if (-not $files -or $files.Count -eq 0) {
    throw "No vcxproj under $BuildDir"
}

$pattern = '<PlatformToolset>[^<]*</PlatformToolset>'
$replacement = "<PlatformToolset>$Toolset</PlatformToolset>"
$count = 0
foreach ($f in $files) {
    $c = Get-Content -LiteralPath $f.FullName -Raw
    $n = [regex]::Replace($c, $pattern, $replacement)
    if ($n -ne $c) {
        Set-Content -LiteralPath $f.FullName -Value $n -NoNewline -Encoding UTF8
        $count++
    }
}
Write-Output "Rewrote PlatformToolset=$Toolset in $count vcxproj file(s)."