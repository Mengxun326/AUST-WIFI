# Android App

This folder contains the Android port of AUST WiFi. It is a Qt Quick mobile app
for campus-network login, background guarding, and APK self-update handoff.

## Current Scope

- Uses the teacher account first when both teacher user and password are filled.
- Falls back to the student account and selected carrier server.
- Supports optional login on app launch.
- Refreshes WiFi status every 15 seconds in the app foreground, or from the
  foreground guardian service when background guarding is enabled.
- Requests Android WiFi-identification permissions from the app UI.
- Uses the shared app logo for Android launcher icons and the mobile header.
- Provides a foreground guardian service toggle with notification permission
  request UI. The service now sends periodic guard ticks into the Qt backend,
  reusing the existing WiFi detection and auto-login scheduler.
- Checks the Android update manifest, compares APK `versionCode`, downloads a
  newer APK, verifies SHA-256, and hands it off to Android's system installer.
- Uses the same login endpoint family as the desktop app.
- Allows cleartext HTTP traffic because the campus gateway is `http://10.255.0.19`.
- Stores passwords through Android Keystore backed AES-GCM encryption.

## Build Environment Used

- Qt: `E:\QT\6.11.1`
- Qt kit: `android_arm64_v8a`
- JDK: `C:\Program Files\Java\jdk-21`
- Android SDK: `D:\Android SDK`
- Android platform: `platforms;android-36` installed, NDK compile platform set to
  `android-35`
- Android NDK: `27.2.12479018` / r27c
- ABI: `arm64-v8a`

Qt/qmake did not handle the spaced SDK path reliably, so the build uses the
Windows short path for `D:\Android SDK`.

## Build

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_android_apk.ps1
```

The debug APK is generated at:

```text
build\android-arm64\android-build\AUST_WIFI_ANDROID.apk
```

Release packaging, signing, update manifest generation, and optional server
upload are handled by:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-android.ps1 -Notes "Android update notes."
```

See `docs\ANDROID_RELEASE.md` for first-time signing key setup and upload
commands.

Qt's Gradle wrapper uses Gradle 9.3.1. The build script points the generated
wrapper to the Huawei Cloud Gradle mirror by default because the official GitHub
release download was unstable on this machine.

## Next Mobile Milestones

- Broaden real-device regression testing across Android vendors.
- Add device-side screenshots for the user manual.
- Consider a smaller update-card release notes view after v1.0.
