# Android MVP

This folder contains the first Android port of AUST WiFi. It is a Qt Quick
mobile MVP that can save student and teacher credentials and manually submit a
campus-network login request.

## Current Scope

- Uses the teacher account first when both teacher user and password are filled.
- Falls back to the student account and selected carrier server.
- Uses the same login endpoint family as the desktop app.
- Allows cleartext HTTP traffic because the campus gateway is `http://10.255.0.19`.
- Stores passwords through the existing `CredentialStore`; Android currently uses
  the non-Windows QSettings fallback, so this should be improved before a public
  mobile release.

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

Qt's Gradle wrapper uses Gradle 9.3.1. The build script points the generated
wrapper to the Huawei Cloud Gradle mirror by default because the official GitHub
release download was unstable on this machine.

## Next Mobile Milestones

- Add Android Keystore based password storage.
- Add WiFi SSID/network-state detection and automatic login scheduling.
- Add a signed release APK build path.
- Add an APK update manifest on `meng-xun.top`.
- Add device-side testing notes and screenshots.
