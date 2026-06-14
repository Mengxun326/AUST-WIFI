param(
    [string]$QtVersionRoot = "E:\QT\6.11.1",
    [string]$AndroidSdkRoot = "D:\Android SDK",
    [string]$AndroidNdkVersion = "27.2.12479018",
    [string]$JavaHome = "C:\Program Files\Java\jdk-21",
    [string]$Abi = "arm64-v8a",
    [string]$AndroidPlatform = "android-35",
    [string]$AndroidBuildToolsVersion = "36.0.0",
    [string]$GradleDistributionUrl = "https://mirrors.huaweicloud.com/gradle/gradle-9.3.1-bin.zip"
)

$ErrorActionPreference = "Stop"

function Resolve-ShortPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path $Path)) {
        throw "Path not found: $Path"
    }

    $escapedPath = $Path.Replace('"', '\"')
    $shortPath = (cmd /c "for %I in (`"$escapedPath`") do @echo %~sI").Trim()
    if ([string]::IsNullOrWhiteSpace($shortPath)) {
        return $Path
    }
    return $shortPath
}

function Invoke-Logged {
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

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDir = Join-Path $repoRoot "build\android-arm64"
$projectFile = Join-Path $repoRoot "android\AUST_WIFI_ANDROID.pro"
$androidBuildDir = Join-Path $buildDir "android-build"
$deploymentSettings = Join-Path $buildDir "android-AUST_WIFI_ANDROID-deployment-settings.json"
$copiedApk = Join-Path $androidBuildDir "AUST_WIFI_ANDROID.apk"
$gradleApk = Join-Path $androidBuildDir "build\outputs\apk\debug\android-build-debug.apk"

if ($Abi -ne "arm64-v8a") {
    throw "This script currently targets arm64-v8a because the qmake kit path is Qt android_arm64_v8a."
}

$sdkRootForQt = Resolve-ShortPath $AndroidSdkRoot
$ndkRootForQt = Resolve-ShortPath (Join-Path $AndroidSdkRoot "ndk\$AndroidNdkVersion")

$qmake = Join-Path $QtVersionRoot "android_arm64_v8a\bin\qmake.bat"
$androidDeployQt = Join-Path $QtVersionRoot "mingw_64\bin\androiddeployqt.exe"
$make = "E:\QT\Tools\mingw1310_64\bin\mingw32-make.exe"

foreach ($path in @($qmake, $androidDeployQt, $make, $projectFile, $JavaHome)) {
    if (-not (Test-Path $path)) {
        throw "Required path not found: $path"
    }
}

New-Item -ItemType Directory -Force $buildDir | Out-Null

$env:JAVA_HOME = $JavaHome
$env:ANDROID_HOME = $sdkRootForQt
$env:ANDROID_SDK_ROOT = $sdkRootForQt
$env:ANDROID_NDK_ROOT = $ndkRootForQt
$env:ANDROID_NDK_HOME = $ndkRootForQt
$env:ANDROID_NDK_HOST = "windows-x86_64"
$env:ANDROID_NDK_PLATFORM = $AndroidPlatform
$env:ANDROID_BUILD_TOOLS_REVISION = $AndroidBuildToolsVersion
$env:SKIP_JDK_VERSION_CHECK = "1"
$env:PATH = "$JavaHome\bin;$sdkRootForQt\platform-tools;E:\QT\Tools\mingw1310_64\bin;$env:PATH"

Invoke-Logged $qmake @($projectFile) $buildDir
Invoke-Logged $make @("apk_install_target") $buildDir
Invoke-Logged $androidDeployQt @(
    "--input", $deploymentSettings,
    "--output", $androidBuildDir,
    "--aux-mode",
    "--android-platform", $AndroidPlatform
) $buildDir

if (-not [string]::IsNullOrWhiteSpace($GradleDistributionUrl)) {
    $wrapperProperties = Join-Path $androidBuildDir "gradle\wrapper\gradle-wrapper.properties"
    if (Test-Path $wrapperProperties) {
        $escapedUrl = $GradleDistributionUrl.Replace(":", "\:")
        $content = Get-Content $wrapperProperties
        $content = $content -replace "^distributionUrl=.*$", "distributionUrl=$escapedUrl"
        Set-Content -Encoding ASCII -Path $wrapperProperties -Value $content
    }
}

$gradleProperties = Join-Path $androidBuildDir "gradle.properties"
if (Test-Path $gradleProperties) {
    $content = Get-Content $gradleProperties
    $content = $content -replace "^androidBuildToolsVersion=.*$", "androidBuildToolsVersion=$AndroidBuildToolsVersion"
    Set-Content -Encoding ASCII -Path $gradleProperties -Value $content
}

Invoke-Logged (Join-Path $androidBuildDir "gradlew.bat") @("--no-daemon", "assembleDebug") $androidBuildDir

if (-not (Test-Path $gradleApk)) {
    throw "APK was not generated: $gradleApk"
}

Copy-Item -Force $gradleApk $copiedApk

$apk = Get-Item $copiedApk
Write-Host "APK generated: $($apk.FullName)"
Write-Host "Size: $($apk.Length) bytes"
