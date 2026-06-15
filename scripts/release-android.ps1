param(
    [string]$Version,
    [string]$VersionCode,
    [string]$Notes = "Android update notes.",
    [string]$UpdateBaseUrl = "https://www.meng-xun.top/aust-wifi/android",
    [string]$QtVersionRoot = "E:\QT\6.11.1",
    [string]$AndroidSdkRoot = "D:\Android SDK",
    [string]$AndroidNdkVersion = "27.2.12479018",
    [string]$JavaHome = "C:\Program Files\Java\jdk-21",
    [string]$Abi = "arm64-v8a",
    [string]$AndroidPlatform = "android-35",
    [string]$AndroidBuildToolsVersion = "36.0.0",
    [string]$GradleDistributionUrl = "https://mirrors.huaweicloud.com/gradle/gradle-9.3.1-bin.zip",
    [string]$KeystorePath = $env:AUST_WIFI_ANDROID_KEYSTORE,
    [string]$KeyAlias = $env:AUST_WIFI_ANDROID_KEY_ALIAS,
    [string]$KeystorePassword = $env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD,
    [string]$KeyPassword = $env:AUST_WIFI_ANDROID_KEY_PASSWORD,
    [switch]$Unsigned,
    [switch]$Upload,
    [string]$UploadHost = "47.121.180.250",
    [string]$UploadUser,
    [int]$UploadPort = 32208,
    [string]$UploadRemoteRoot = "/www/wwwroot/47_121_180_250/aust-wifi/android",
    [string]$UploadIdentityFile,
    [switch]$AllowUnsignedUpload,
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

function Resolve-AndroidBuildTool {
    param([string]$CommandName)

    $candidate = Join-Path $AndroidSdkRoot "build-tools\$AndroidBuildToolsVersion\$CommandName"
    if (Test-Path $candidate) {
        return (Resolve-Path $candidate).Path
    }

    $buildToolsRoot = Join-Path $AndroidSdkRoot "build-tools"
    $fallback = Get-ChildItem -Path $buildToolsRoot -Recurse -Filter $CommandName -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($fallback) {
        return $fallback.FullName
    }

    throw "Android build tool not found: $CommandName"
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

    $content = Get-Content $Path -Raw -Encoding UTF8
    $updated = [System.Text.RegularExpressions.Regex]::Replace($content, $Pattern, $Replacement)
    if ($updated -ne $content) {
        $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($Path, $updated, $utf8NoBom)
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

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$ArgumentList,
        [string]$WorkingDirectory = (Get-Location).Path
    )

    Write-Host ">> $FilePath $($ArgumentList -join ' ')"
    Push-Location $WorkingDirectory
    try {
        & $FilePath @ArgumentList
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }
}

function Read-AndroidVersionInfo {
    param([string]$ProjectFilePath)

    $project = Get-Content $ProjectFilePath -Raw -Encoding UTF8
    if ($project -notmatch '(?m)^\s*ANDROID_VERSION_NAME\s*=\s*(\S+)\s*$') {
        throw "Cannot read ANDROID_VERSION_NAME from $ProjectFilePath"
    }
    $name = $Matches[1].Trim()

    if ($project -notmatch '(?m)^\s*ANDROID_VERSION_CODE\s*=\s*(\d+)\s*$') {
        throw "Cannot read ANDROID_VERSION_CODE from $ProjectFilePath"
    }
    $code = [int]$Matches[1]

    return [PSCustomObject]@{
        Name = $name
        Code = $code
    }
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
Set-Location $repoRoot

$projectFile = Join-Path $repoRoot "android\AUST_WIFI_ANDROID.pro"
$buildScript = Join-Path $repoRoot "scripts\build_android_apk.ps1"
$androidBuildDir = Join-Path $repoRoot "build\android-arm64\android-build"
$unsignedApk = Join-Path $androidBuildDir "AUST_WIFI_ANDROID-release-unsigned.apk"
$outputDir = Join-Path $repoRoot "installer\android"
$manifestPath = Join-Path $outputDir "update.json"

foreach ($path in @($projectFile, $buildScript, $JavaHome)) {
    if (-not (Test-Path $path)) {
        throw "Required path not found: $path"
    }
}

$versionInfo = Read-AndroidVersionInfo -ProjectFilePath $projectFile
$currentVersion = $versionInfo.Name
$currentVersionCode = $versionInfo.Code

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = $currentVersion
}

if ($Version -notmatch '^\d+\.\d+\.\d+([.-][A-Za-z0-9]+)?$') {
    throw "Version must look like 0.6.0 or 0.6.0-beta: $Version"
}

if ($Version -ne $currentVersion -and [string]::IsNullOrWhiteSpace($VersionCode)) {
    throw "VersionCode is required when Version changes. Android updates need a monotonically increasing version code."
}

if (-not [string]::IsNullOrWhiteSpace($VersionCode)) {
    if ($VersionCode -notmatch '^\d+$') {
        throw "VersionCode must be an integer: $VersionCode"
    }
    $currentVersionCode = [int]$VersionCode
    Update-TextFile -Path $projectFile `
        -Pattern '(?m)^(\s*ANDROID_VERSION_CODE\s*=\s*)\d+\s*$' `
        -Replacement "`${1}$currentVersionCode"
}

if ($Version -ne $currentVersion) {
    Update-TextFile -Path $projectFile `
        -Pattern '(?m)^(\s*ANDROID_VERSION_NAME\s*=\s*)\S+\s*$' `
        -Replacement "`${1}$Version"
}

Assert-ChildPath -Root $repoRoot -Path $outputDir
if ($Clean -and (Test-Path $outputDir)) {
    Remove-Item -LiteralPath $outputDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "AUST WiFi Android release"
Write-Host "Version: $Version"
Write-Host "Version code: $currentVersionCode"
Write-Host "Build type: Release"

Invoke-Checked "powershell" @(
    "-ExecutionPolicy", "Bypass",
    "-File", $buildScript,
    "-QtVersionRoot", $QtVersionRoot,
    "-AndroidSdkRoot", $AndroidSdkRoot,
    "-AndroidNdkVersion", $AndroidNdkVersion,
    "-JavaHome", $JavaHome,
    "-Abi", $Abi,
    "-AndroidPlatform", $AndroidPlatform,
    "-AndroidBuildToolsVersion", $AndroidBuildToolsVersion,
    "-BuildType", "Release",
    "-GradleDistributionUrl", $GradleDistributionUrl
) $repoRoot

if (-not (Test-Path $unsignedApk)) {
    throw "Release APK was not generated: $unsignedApk"
}

$releaseApkName = "AUST-WIFI-Android-$Version.apk"
$releaseApkPath = Join-Path $outputDir $releaseApkName
$isSigned = -not [bool]$Unsigned

if ($Unsigned) {
    $releaseApkName = "AUST-WIFI-Android-$Version-unsigned.apk"
    $releaseApkPath = Join-Path $outputDir $releaseApkName
    Copy-Item -LiteralPath $unsignedApk -Destination $releaseApkPath -Force
    Write-Warning "Generated unsigned APK. Android devices cannot install it as a production update."
} else {
    if ([string]::IsNullOrWhiteSpace($KeystorePath)) {
        throw "KeystorePath is required. Set AUST_WIFI_ANDROID_KEYSTORE or pass -KeystorePath. Use -Unsigned only for local packaging checks."
    }
    if ([string]::IsNullOrWhiteSpace($KeyAlias)) {
        throw "KeyAlias is required. Set AUST_WIFI_ANDROID_KEY_ALIAS or pass -KeyAlias."
    }
    if ([string]::IsNullOrWhiteSpace($KeystorePassword)) {
        throw "KeystorePassword is required. Set AUST_WIFI_ANDROID_KEYSTORE_PASSWORD or pass -KeystorePassword."
    }

    $keystoreFullPath = Resolve-InputFilePath -Path $KeystorePath
    if (-not (Test-Path $keystoreFullPath)) {
        throw "Keystore not found: $keystoreFullPath"
    }

    $zipalign = Resolve-AndroidBuildTool -CommandName "zipalign.exe"
    $apksigner = Resolve-AndroidBuildTool -CommandName "apksigner.bat"
    $alignedApkPath = Join-Path $outputDir "AUST-WIFI-Android-$Version-aligned.apk"

    Invoke-Checked $zipalign @("-f", "-p", "4", $unsignedApk, $alignedApkPath) $repoRoot

    $previousStorePassword = $env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD
    $previousKeyPassword = $env:AUST_WIFI_ANDROID_KEY_PASSWORD
    $env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD = $KeystorePassword
    if (-not [string]::IsNullOrWhiteSpace($KeyPassword)) {
        $env:AUST_WIFI_ANDROID_KEY_PASSWORD = $KeyPassword
    }

    try {
        $signArgs = @(
            "sign",
            "--ks", $keystoreFullPath,
            "--ks-key-alias", $KeyAlias,
            "--ks-pass", "env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD",
            "--out", $releaseApkPath
        )
        if (-not [string]::IsNullOrWhiteSpace($KeyPassword)) {
            $signArgs += @("--key-pass", "env:AUST_WIFI_ANDROID_KEY_PASSWORD")
        }
        $signArgs += $alignedApkPath
        Invoke-Checked $apksigner $signArgs $repoRoot
    } finally {
        $env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD = $previousStorePassword
        $env:AUST_WIFI_ANDROID_KEY_PASSWORD = $previousKeyPassword
    }

    Invoke-Checked $apksigner @("verify", "--verbose", $releaseApkPath) $repoRoot
    Remove-Item -LiteralPath $alignedApkPath -Force
}

$hash = (Get-FileHash -Algorithm SHA256 $releaseApkPath).Hash.ToLowerInvariant()
$size = (Get-Item $releaseApkPath).Length
$baseUrl = $UpdateBaseUrl.TrimEnd("/")

$manifest = [ordered]@{
    manifest_version = 1
    platform = "android"
    package = "top.mengxun.austwifi"
    latest = $Version
    version_code = $currentVersionCode
    min_supported_version_code = 1
    abi = $Abi
    url = "$baseUrl/releases/$releaseApkName"
    sha256 = $hash
    size = $size
    published_at = (Get-Date -Format "yyyy-MM-dd")
    notes = $Notes
    signed = [bool]$isSigned
}

$manifestJson = $manifest | ConvertTo-Json
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($manifestPath, $manifestJson + [Environment]::NewLine, $utf8NoBom)

if ($Upload) {
    if ($Unsigned -and -not $AllowUnsignedUpload) {
        throw "Refusing to upload an unsigned APK. Pass -AllowUnsignedUpload only for private testing."
    }
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
    $remoteApkPath = "$remoteReleasesDir/$releaseApkName"
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
    Write-Host "Uploading Android release artifacts..."
    Write-Host "Remote: $remote"
    Write-Host "Remote root: $remoteRoot"

    Invoke-Checked $ssh ($sshArgs + @($remote, ("mkdir -p " + (ConvertTo-RemoteShellArgument -Value $remoteReleasesDir)))) $repoRoot
    Invoke-Checked $scp ($scpArgs + @($releaseApkPath, "${remote}:$remoteApkPath")) $repoRoot
    Invoke-Checked $scp ($scpArgs + @($manifestPath, "${remote}:$remoteManifestPath")) $repoRoot

    Write-Host "Upload completed."
}

Write-Host ""
Write-Host "Android release artifacts generated:"
Write-Host "APK: $releaseApkPath"
Write-Host "Manifest: $manifestPath"
Write-Host ""
Write-Host "Upload to:"
Write-Host "$baseUrl/releases/$releaseApkName"
Write-Host "$baseUrl/update.json"
