param(
    [string]$KeystorePath = "secrets\android-release.jks",
    [string]$Alias = "aust-wifi-android",
    [string]$JavaHome = "C:\Program Files\Java\jdk-21",
    [int]$KeySize = 4096,
    [int]$ValidityDays = 10000,
    [string]$StorePassword = $env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD,
    [string]$KeyPassword = $env:AUST_WIFI_ANDROID_KEY_PASSWORD,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function ConvertTo-PlainText {
    param([securestring]$Value)

    if (-not $Value) {
        return ""
    }

    $credential = New-Object System.Net.NetworkCredential("", $Value)
    return $credential.Password
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
Set-Location $repoRoot

$keytool = Join-Path $JavaHome "bin\keytool.exe"
if (-not (Test-Path $keytool)) {
    throw "keytool not found: $keytool"
}

$keystoreFullPath = if ([System.IO.Path]::IsPathRooted($KeystorePath)) {
    [System.IO.Path]::GetFullPath($KeystorePath)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $repoRoot $KeystorePath))
}

if ((Test-Path $keystoreFullPath) -and -not $Force) {
    throw "Keystore already exists: $keystoreFullPath. Use -Force only if you intentionally want to replace it."
}

if ([string]::IsNullOrWhiteSpace($StorePassword)) {
    $StorePassword = ConvertTo-PlainText (Read-Host "Keystore password" -AsSecureString)
}

if ([string]::IsNullOrWhiteSpace($KeyPassword)) {
    $KeyPassword = $StorePassword
}

if ([string]::IsNullOrWhiteSpace($StorePassword) -or $StorePassword.Length -lt 6) {
    throw "Keystore password must be at least 6 characters."
}

if ([string]::IsNullOrWhiteSpace($KeyPassword) -or $KeyPassword.Length -lt 6) {
    throw "Key password must be at least 6 characters."
}

$keystoreDir = Split-Path $keystoreFullPath -Parent
New-Item -ItemType Directory -Force -Path $keystoreDir | Out-Null

& $keytool `
    -genkeypair `
    -v `
    -keystore $keystoreFullPath `
    -storetype PKCS12 `
    -alias $Alias `
    -keyalg RSA `
    -keysize $KeySize `
    -validity $ValidityDays `
    -storepass $StorePassword `
    -keypass $KeyPassword `
    -dname "CN=AUST WiFi Android, OU=AUST WiFi, O=Mengxun, L=Huainan, ST=Anhui, C=CN"

if ($LASTEXITCODE -ne 0) {
    throw "keytool failed with exit code $LASTEXITCODE"
}

Write-Host "Android release keystore generated:"
Write-Host $keystoreFullPath
Write-Host ""
Write-Host "Set these before running scripts\release-android.ps1:"
Write-Host "`$env:AUST_WIFI_ANDROID_KEYSTORE = `"$keystoreFullPath`""
Write-Host "`$env:AUST_WIFI_ANDROID_KEY_ALIAS = `"$Alias`""
Write-Host "`$env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD = `"your-keystore-password`""
Write-Host "`$env:AUST_WIFI_ANDROID_KEY_PASSWORD = `"your-key-password`""
