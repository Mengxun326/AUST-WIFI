param(
    [string]$PrivateKeyPath = "secrets\update-signing-private.xml",
    [int]$KeySize = 3072,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
Set-Location $repoRoot

$privateKeyFullPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $PrivateKeyPath))
$secretsDir = Split-Path $privateKeyFullPath -Parent
New-Item -ItemType Directory -Force -Path $secretsDir | Out-Null

if ((Test-Path $privateKeyFullPath) -and -not $Force) {
    throw "Private key already exists: $privateKeyFullPath. Use -Force only if you intentionally want to replace it."
}

$rsa = New-Object System.Security.Cryptography.RSACryptoServiceProvider($KeySize)
try {
    $privateXml = $rsa.ToXmlString($true)
    $publicXml = [xml]$rsa.ToXmlString($false)

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($privateKeyFullPath, $privateXml, $utf8NoBom)

    Write-Host "Private key written to:"
    Write-Host $privateKeyFullPath
    Write-Host ""
    Write-Host "Add these public constants to app_config.h:"
    Write-Host "#define APP_UPDATE_SIGNATURE_MODULUS `"$($publicXml.RSAKeyValue.Modulus)`""
    Write-Host "#define APP_UPDATE_SIGNATURE_EXPONENT `"$($publicXml.RSAKeyValue.Exponent)`""
} finally {
    $rsa.Clear()
}
