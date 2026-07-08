# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

AUST-WIFI is a cross-platform Qt auto-reconnect tool for the AUST (安徽理工大学) campus network. It detects WiFi disconnection and automatically logs in via the campus captive portal at `http://10.255.0.19`. Supports both desktop (Windows/Linux) and Android.

## Build & Release

### Desktop (Windows)

```powershell
# Build with Qt Creator (Release config) or command line:
qmake AUST_WIFI.pro
mingw32-make -j$(nproc)  # or make on Linux

# Full Windows release (compile, sign, package Inno Setup, generate update manifest):
powershell -ExecutionPolicy Bypass -File .\scripts\release-windows.ps1 -Clean -Notes "更新说明"
```

Qt: `E:\QT\6.11.1`, compiler: mingw1310_64, Inno Setup 6.

### Android

```powershell
# Debug APK:
powershell -ExecutionPolicy Bypass -File .\scripts\build_android_apk.ps1
# Output: build\android-arm64\android-build\AUST_WIFI_ANDROID.apk

# Release APK + update manifest:
powershell -ExecutionPolicy Bypass -File .\scripts\release-android.ps1 -Notes "更新说明"
```

Qt kit: `android_arm64_v8a`, JDK 21, NDK r27c, target API 35. Build uses Windows short path for Android SDK (`D:\Android SDK`). KDAB `android_openssl` is auto-downloaded and bundled for HTTPS support.

### Linux

```bash
mkdir build && cd build
qmake ../AUST_WIFI.pro
make -j$(nproc)
```

Requires Qt 6.7+ and `wireless-tools` or `network-manager` for SSID detection.

## Project Structure

```
root/
├── AUST_WIFI.pro              # Desktop qmake project
├── main.cpp                   # Desktop entry point
├── mainwindow.h/cpp           # System tray + window + orchestration
├── wifimanager.h/cpp          # Core: WiFi detection, login, auto-reconnect state machine
├── config_dialog.h/cpp        # Credential config dialog (student + teacher)
├── credentialstore.h/cpp      # Platform password encryption (DPAPI/Keystore/fallback)
├── updatemanager.h/cpp        # Auto-update: manifest fetch, download, SHA-256 verify
├── updatesignature.h/cpp      # RSA-SHA256 manifest signature verification (Win CryptAPI)
├── uistyles.h/cpp             # Qt stylesheets for main window and config dialog
├── app_config.h               # Compile-time constants: version, update URL, signing public key
├── mainwindow.ui / config_dialog.ui
├── resources.qrc / icons/
│
├── android/
│   ├── AUST_WIFI_ANDROID.pro  # Android qmake project
│   ├── main.cpp               # Android entry (QGuiApplication + QQmlApplicationEngine)
│   ├── mobilebackend.h/cpp    # QML backend — all business logic (~1400 lines)
│   ├── qml/Main.qml           # Main UI
│   ├── assets/logo.png
│   └── android/
│       ├── AndroidManifest.xml
│       ├── res/
│       └── src/top/mengxun/austwifi/
│           ├── NetworkStateHelper.java       # WiFi state + SSID reading via Android API
│           ├── SecureCredentialStore.java    # AES-GCM via Android Keystore
│           ├── AustWifiForegroundService.java # Foreground guardian with 15s tick
│           └── ApkUpdateHelper.java          # Version info + install intent
│
├── scripts/
│   ├── build_android_apk.ps1     # Debug APK build
│   ├── release-windows.ps1       # Full Windows release pipeline
│   ├── release-android.ps1       # Full Android release pipeline
│   ├── new-update-signing-key.ps1
│   └── new-android-signing-key.ps1
│
├── installer/                    # Generated installers and update.json
├── docs/
│   ├── LINUX_BUILD.md
│   ├── AUTO_UPDATE.md            # Update server setup + manifest format
│   ├── ANDROID_RELEASE.md
│   └── 用户说明书.md
│
└── secrets/                      # .gitignored: signing keys
```

## Architecture

### Desktop

Single-threaded Qt event loop. `MainWindow` owns three core objects:

1. **WiFiManager** — The core. Runs a 1-second `QTimer` that:
   - Checks internet connectivity via `HEAD http://www.baidu.com` (8s timeout)
   - If disconnected and not in grace period (15s after last login success), triggers login
   - Detects WiFi SSID via `QProcess` spawning `netsh wlan show interfaces` (Windows) or `iwgetid -r` (Linux), with PowerShell/nmcli fallbacks
   - Dual credential system: student account (username + password + carrier: aust/unicom/cmcc) and teacher account (username + password, server always "jzg"). Teacher takes priority when both are configured, regardless of SSID.
   - State machine: `Idle → CheckingNetwork → DetectingSSID → LoggingIn → Connected/Disconnected/CoolingDown/PausedByUser/Configuring`
   - Retry: 3 attempts with 3s/6s/9s backoff, then 30s cooldown. Login in progress blocks concurrent attempts; stuck logins are force-cleaned after 15s.

2. **ConfigDialog** — Opens modally. Opening it calls `WiFiManager::pauseNetworkOperations()` to stop the timer and abort in-flight requests. Saving triggers `resumeNetworkOperations()` and a delayed connectivity check.

3. **UpdateManager** — Fetches signed `update.json` from `APP_UPDATE_MANIFEST_URL`, parses the Base64-encoded RSA-signed payload, compares versions via semantic comparison, downloads the installer, verifies SHA-256, launches with `/SILENT`.

**CredentialStore** — Platform password protection:
- Windows: DPAPI (`CryptProtectData`/`CryptUnprotectData`) with scope-specific entropy
- Android: Delegates to `SecureCredentialStore.java` (AES-256-GCM via Android Keystore)
- Linux: QSettings INI file plaintext (with `secureBackendAvailable() == false`)

**UpdateSignature** — Uses Windows CryptoAPI to verify RSA-SHA256 PKCS#1 v1.5 signatures. Public key is hardcoded in `app_config.h`. Returns false on non-Windows platforms (not implemented).

### Login Protocol

Both student and teacher share the same query builder (`buildLoginQuery`):
```
callback=dr1003&DDDDD={user}@{server}&upass={password}&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=
```

- **Student**: POST to `http://10.255.0.19/a79.htm` (Content-Type: application/x-www-form-urlencoded)
- **Teacher**: GET to `http://10.255.0.19/drcom/login?...`

Login success detection checks response for: `"result":1`, `login_ok`, or HTML containing `UID=` + `@jzg`.

### Android

QML app with `MobileBackend` as the single C++ backend exposed to QML via `rootContext()->setContextProperty("backend", ...)`. Key differences from desktop:

- **WiFi binding**: Before login, calls `NetworkStateHelper.bindWifiNetwork()` to acquire a `Network` object and bind the HTTP request to it — this prevents mobile data from hijacking the campus-network login request.
- **Campus detection**: Probes `http://10.255.0.19/` to detect campus gateway reachability, allowing custom router SSIDs (not just official `AUST_Student`/`AUST_Faculty`) to trigger auto-login.
- **Foreground guardian**: `AustWifiForegroundService` sends a 15s tick into `MobileBackend`, which reuses the same WiFi detection and auto-login logic.
- **Auto-login timing**: Delayed start (2.5s after launch), 45s cooldown between attempts, 2 transient retries, then backs off.
- **APK update**: Fetches `android/update.json`, compares `version_code`, downloads APK, verifies SHA-256, hands off to system installer via `FileProvider` URI.
- **Android version**: `ANDROID_VERSION_CODE` and `ANDROID_VERSION_NAME` in `AUST_WIFI_ANDROID.pro`. `VersionCode` must be monotonically increasing.

### Update Manifest Format

```json
{
  "latest": "4.0.0",
  "url": "https://...",
  "sha256": "...",
  "size": 10485760,
  "published_at": "2026-06-14",
  "notes": "...",
  "force": false,
  "payload": "<Base64 of compact JSON of the above fields>",
  "signature": "<Base64 RSA-SHA256 signature of payload bytes>"
}
```

When `APP_REQUIRE_SIGNED_MANIFEST` is 1 (current default), unsigned manifests are rejected. The `payload` field is parsed and takes precedence over top-level fields. Installer URL must use HTTPS.

## Key Configuration

- `APP_VERSION` in `app_config.h` — also synced in `APP_VERSION.txt` and `setup.iss`
- `APP_UPDATE_MANIFEST_URL` — `https://www.meng-xun.top/aust-wifi/update.json`
- `APP_UPDATE_SIGNATURE_MODULUS` / `APP_UPDATE_SIGNATURE_EXPONENT` — RSA public key for manifest verification
- Android update manifest: `https://www.meng-xun.top/aust-wifi/android/update.json`
- QSettings org/app: `"AUST_WIFI"` / `"Config"` on all platforms
- Passwords stored under QSettings keys: `student/password_dpapi`, `teacher/password_dpapi` (or `password_dpapi` for legacy)

## Notes

- Debug output is disabled in desktop builds via `DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_INFO_OUTPUT` in the .pro file. Remove to re-enable.
- The `USE_LIBCURL` path exists as an alternative to Qt's QNetworkAccessManager but is not the default (`USE_QT_NETWORK` is).
- Desktop app checks for system tray availability on startup; exits with error if unavailable.
- First-run detection: no `student/user` and no `teacher/user` in QSettings → shows window instead of minimizing to tray.
- Android allows cleartext HTTP traffic (`android:usesCleartextTraffic="true"`) because the campus gateway is HTTP-only.
- The Gradle wrapper uses Huawei Cloud mirror by default due to GitHub download instability on the dev machine.
- Update server uses port 32208 for SSH/SCP upload.
