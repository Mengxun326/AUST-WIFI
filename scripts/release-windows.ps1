param(
    [string]$Version,
    [string]$MinSupportedVersion,
    [string]$Notes = "Update notes.",
    [string]$UpdateBaseUrl = "https://www.meng-xun.top/aust-wifi",
    [string]$QtBin = "E:\QT\6.11.1\mingw_64\bin",
    [string]$MingwBin = "E:\QT\Tools\mingw1310_64\bin",
    [string]$InnoCompiler,
    [switch]$Force,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

function Resolve-Tool {
    param(
        [string]$PreferredPath,
        [string]$CommandName
    )

    if ($PreferredPath) {
        $candidate = Join-Path $PreferredPath $CommandName
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $command = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "Tool not found: $CommandName"
}

function Assert-ChildPath {
    param(
        [string]$Root,
        [string]$Path
    )

    $rootPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    $targetPath = [System.IO.Path]::GetFullPath($Path)
    if (-not $targetPath.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the repository: $targetPath"
    }
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
Set-Location $repoRoot

if (-not $InnoCompiler) {
    $programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
    if ($programFilesX86) {
        $InnoCompiler = Join-Path $programFilesX86 "Inno Setup 6\ISCC.exe"
    } else {
        $InnoCompiler = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    }
}

$appConfigPath = Join-Path $repoRoot "app_config.h"
if (-not (Test-Path $appConfigPath)) {
    throw "Missing app_config.h"
}

$appConfig = Get-Content $appConfigPath -Raw
if (-not $Version) {
    if ($appConfig -match '#define\s+APP_VERSION\s+"([^"]+)"') {
        $Version = $Matches[1]
    } else {
        throw "Cannot read APP_VERSION from app_config.h"
    }
}

if (-not $MinSupportedVersion) {
    $MinSupportedVersion = "3.0.0"
}

$qmake = Resolve-Tool -PreferredPath $QtBin -CommandName "qmake.exe"
$windeployqt = Resolve-Tool -PreferredPath $QtBin -CommandName "windeployqt.exe"
$make = Resolve-Tool -PreferredPath $MingwBin -CommandName "mingw32-make.exe"

if (-not (Test-Path $InnoCompiler)) {
    throw "Inno Setup compiler not found: $InnoCompiler"
}

$qtBinResolved = Split-Path $qmake -Parent
$mingwBinResolved = Split-Path $make -Parent
$env:PATH = "$mingwBinResolved;$qtBinResolved;$env:PATH"

$buildDir = Join-Path $repoRoot "build\release-windows"
$distDir = Join-Path $repoRoot "dist\AUST_WIFI"
$installerDir = Join-Path $repoRoot "installer"
$projectFile = Join-Path $repoRoot "AUST_WIFI.pro"
$setupScript = Join-Path $repoRoot "setup.iss"

Assert-ChildPath -Root $repoRoot -Path $buildDir
Assert-ChildPath -Root $repoRoot -Path $distDir
Assert-ChildPath -Root $repoRoot -Path $installerDir

Write-Host "AUST WiFi Windows release"
Write-Host "Version: $Version"
Write-Host "qmake: $qmake"
Write-Host "make: $make"
Write-Host "windeployqt: $windeployqt"
Write-Host "Inno Setup: $InnoCompiler"

if ($Clean) {
    foreach ($path in @($buildDir, $distDir, $installerDir)) {
        if (Test-Path $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

New-Item -ItemType Directory -Force -Path $buildDir, $distDir, $installerDir | Out-Null

Push-Location $buildDir
try {
    & $qmake $projectFile "CONFIG+=release"
    if ($LASTEXITCODE -ne 0) {
        throw "qmake failed."
    }

    & $make
    if ($LASTEXITCODE -ne 0) {
        throw "mingw32-make failed."
    }
} finally {
    Pop-Location
}

$builtExe = Join-Path $buildDir "release\AUST_WIFI.exe"
if (-not (Test-Path $builtExe)) {
    throw "Built executable not found: $builtExe"
}

Copy-Item -LiteralPath $builtExe -Destination (Join-Path $distDir "AUST_WIFI.exe") -Force

& $windeployqt "--release" "--compiler-runtime" (Join-Path $distDir "AUST_WIFI.exe")
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed."
}

& $InnoCompiler "/DMyAppVersion=$Version" "/DSourceDir=$distDir" "/DInstallerOutputDir=$installerDir" $setupScript
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup packaging failed."
}

$installerPath = Join-Path $installerDir "AUST-WIFI-Setup-$Version.exe"
if (-not (Test-Path $installerPath)) {
    throw "Installer not found: $installerPath"
}

$hash = (Get-FileHash -Algorithm SHA256 $installerPath).Hash.ToLowerInvariant()
$size = (Get-Item $installerPath).Length
$manifestPath = Join-Path $installerDir "update.json"

$manifest = [ordered]@{
    latest = $Version
    min_supported = $MinSupportedVersion
    url = "$UpdateBaseUrl/releases/AUST-WIFI-Setup-$Version.exe"
    sha256 = $hash
    size = $size
    published_at = (Get-Date -Format "yyyy-MM-dd")
    notes = $Notes
    force = [bool]$Force
}

$manifestJson = $manifest | ConvertTo-Json
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($manifestPath, $manifestJson + [Environment]::NewLine, $utf8NoBom)

Write-Host ""
Write-Host "Release artifacts generated:"
Write-Host "Installer: $installerPath"
Write-Host "Manifest: $manifestPath"
Write-Host ""
Write-Host "Upload to:"
Write-Host "$UpdateBaseUrl/releases/AUST-WIFI-Setup-$Version.exe"
Write-Host "$UpdateBaseUrl/update.json"
