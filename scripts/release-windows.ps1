param(
    [string]$Version,
    [string]$VersionFile = "APP_VERSION.txt",
    [string]$MinSupportedVersion,
    [string]$Notes = "Update notes.",
    [string]$UpdateBaseUrl = "https://www.meng-xun.top/aust-wifi",
    [string]$QtBin = "E:\QT\6.11.1\mingw_64\bin",
    [string]$MingwBin = "E:\QT\Tools\mingw1310_64\bin",
    [string]$InnoCompiler,
    [string]$SigningKeyPath = "secrets\update-signing-private.xml",
    [switch]$Upload,
    [string]$UploadHost = "47.121.180.250",
    [string]$UploadUser,
    [int]$UploadPort = 32208,
    [string]$UploadRemoteRoot = "/www/wwwroot/www.meng-xun.top/aust-wifi",
    [string]$UploadIdentityFile,
    [switch]$Force,
    [switch]$Clean,
    [switch]$NoSyncVersion,
    [switch]$UnsignedManifest
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

function Update-TextFile {
    param(
        [string]$Path,
        [string]$Pattern,
        [string]$Replacement
    )

    $content = Get-Content $Path -Raw
    $updated = [System.Text.RegularExpressions.Regex]::Replace($content, $Pattern, $Replacement)
    if ($updated -ne $content) {
        $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($Path, $updated, $utf8NoBom)
    }
}

function ConvertTo-CompactJson {
    param([object]$Value)
    return ($Value | ConvertTo-Json -Compress)
}

function New-ManifestSignature {
    param(
        [string]$PayloadJson,
        [string]$PrivateKeyPath
    )

    if (-not (Test-Path $PrivateKeyPath)) {
        throw "Signing private key not found: $PrivateKeyPath. Run scripts\new-update-signing-key.ps1 first, or pass -UnsignedManifest for local testing only."
    }

    $privateXml = Get-Content $PrivateKeyPath -Raw
    $rsa = New-Object System.Security.Cryptography.RSACryptoServiceProvider
    try {
        $rsa.FromXmlString($privateXml)
        $payloadBytes = [System.Text.Encoding]::UTF8.GetBytes($PayloadJson)
        $sha256Oid = [System.Security.Cryptography.CryptoConfig]::MapNameToOID("SHA256")
        $signatureBytes = $rsa.SignData($payloadBytes, $sha256Oid)
        return [Convert]::ToBase64String($signatureBytes)
    } finally {
        $rsa.Clear()
    }
}

function Resolve-InputFilePath {
    param([string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

function ConvertTo-RemoteShellArgument {
    param([string]$Value)

    return "'" + $Value.Replace("'", "'`"'`"'") + "'"
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

$versionFilePath = Join-Path $repoRoot $VersionFile
$appConfig = Get-Content $appConfigPath -Raw
if (-not $Version) {
    if (Test-Path $versionFilePath) {
        $Version = (Get-Content $versionFilePath -Raw).Trim()
    } elseif ($appConfig -match '#define\s+APP_VERSION\s+"([^"]+)"') {
        $Version = $Matches[1].Trim()
    } else {
        throw "Cannot read version from $VersionFile or app_config.h"
    }
}

if ($Version -notmatch '^\d+\.\d+\.\d+([.-][A-Za-z0-9]+)?$') {
    throw "Version must look like 4.0.0 or 4.0.0-beta: $Version"
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

if (-not $NoSyncVersion) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($versionFilePath, $Version + [Environment]::NewLine, $utf8NoBom)
    Update-TextFile -Path $appConfigPath `
        -Pattern '#define\s+APP_VERSION\s+"[^"]+"' `
        -Replacement "#define APP_VERSION `"$Version`""
    Update-TextFile -Path $setupScript `
        -Pattern '#define\s+MyAppVersion\s+"[^"]+"' `
        -Replacement "#define MyAppVersion `"$Version`""
}

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
$signingKeyFullPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $SigningKeyPath))

$payload = [ordered]@{
    latest = $Version
    min_supported = $MinSupportedVersion
    url = "$UpdateBaseUrl/releases/AUST-WIFI-Setup-$Version.exe"
    sha256 = $hash
    size = $size
    published_at = (Get-Date -Format "yyyy-MM-dd")
    notes = $Notes
    force = [bool]$Force
}

$manifest = [ordered]@{}
foreach ($key in $payload.Keys) {
    $manifest[$key] = $payload[$key]
}

if (-not $UnsignedManifest) {
    $payloadJson = ConvertTo-CompactJson -Value $payload
    $payloadBytes = [System.Text.Encoding]::UTF8.GetBytes($payloadJson)
    $manifest["manifest_version"] = 2
    $manifest["signature_algorithm"] = "rsa-sha256-pkcs1-v1_5"
    $manifest["payload"] = [Convert]::ToBase64String($payloadBytes)
    $manifest["signature"] = New-ManifestSignature -PayloadJson $payloadJson -PrivateKeyPath $signingKeyFullPath
} else {
    Write-Warning "Generating unsigned update manifest. V4 clients with APP_REQUIRE_SIGNED_MANIFEST=1 will reject it."
}

$manifestJson = $manifest | ConvertTo-Json
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($manifestPath, $manifestJson + [Environment]::NewLine, $utf8NoBom)

if ($Upload) {
    if (-not $UploadUser) {
        throw "UploadUser is required when -Upload is set. Example: -UploadUser root"
    }

    if (-not $UploadRemoteRoot.StartsWith("/")) {
        throw "UploadRemoteRoot must be an absolute Linux path: $UploadRemoteRoot"
    }

    $ssh = Resolve-Tool -PreferredPath $null -CommandName "ssh.exe"
    $scp = Resolve-Tool -PreferredPath $null -CommandName "scp.exe"
    $remote = "$UploadUser@$UploadHost"
    $remoteRoot = $UploadRemoteRoot.TrimEnd("/")
    $remoteReleasesDir = "$remoteRoot/releases"
    $remoteInstallerPath = "$remoteReleasesDir/AUST-WIFI-Setup-$Version.exe"
    $remoteManifestPath = "$remoteRoot/update.json"

    $sshArgs = @("-p", [string]$UploadPort)
    $scpArgs = @("-P", [string]$UploadPort)

    if ($UploadIdentityFile) {
        $uploadIdentityFullPath = Resolve-InputFilePath -Path $UploadIdentityFile
        if (-not (Test-Path $uploadIdentityFullPath)) {
            throw "Upload identity file not found: $uploadIdentityFullPath"
        }

        $sshArgs += @("-i", $uploadIdentityFullPath)
        $scpArgs += @("-i", $uploadIdentityFullPath)
    }

    Write-Host ""
    Write-Host "Uploading release artifacts..."
    Write-Host "Remote: $remote"
    Write-Host "Remote root: $remoteRoot"

    & $ssh @sshArgs $remote ("mkdir -p " + (ConvertTo-RemoteShellArgument -Value $remoteReleasesDir))
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create remote release directory."
    }

    & $scp @scpArgs $installerPath "${remote}:$remoteInstallerPath"
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to upload installer."
    }

    & $scp @scpArgs $manifestPath "${remote}:$remoteManifestPath"
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to upload update manifest."
    }

    Write-Host "Upload completed."
}

Write-Host ""
Write-Host "Release artifacts generated:"
Write-Host "Installer: $installerPath"
Write-Host "Manifest: $manifestPath"
Write-Host ""
Write-Host "Upload to:"
Write-Host "$UpdateBaseUrl/releases/AUST-WIFI-Setup-$Version.exe"
Write-Host "$UpdateBaseUrl/update.json"
