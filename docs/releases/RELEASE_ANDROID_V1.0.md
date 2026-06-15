# AUST WiFi Android v1.0.0

## Release Scope

- Student and teacher account login, with teacher credentials taking priority
  whenever both teacher user and password are filled.
- Android Keystore backed password storage.
- WiFi status detection and campus WiFi recognition.
- Optional auto-login on app launch and background guard checks.
- Foreground guardian service with notification permission handling.
- Android update checking from `meng-xun.top`, APK download, SHA-256
  verification, package/version validation, and system installer handoff.
- Signed release packaging and server upload workflow.

## Release Artifacts

```text
installer\android\AUST-WIFI-Android-1.0.0.apk
installer\android\update.json
```

Published URLs:

```text
https://www.meng-xun.top/aust-wifi/android/releases/AUST-WIFI-Android-1.0.0.apk
https://www.meng-xun.top/aust-wifi/android/update.json
```

## Required Checks

- Install the signed APK on a clean Android device.
- Confirm the app shows version `1.0.0 (10)` in the update card.
- Grant WiFi identification, notification, and install-unknown-app permissions.
- Save student credentials and manually login on campus WiFi.
- Save teacher credentials and confirm teacher login takes priority.
- Enable auto-login and background guarding, then test app foreground,
  background, and locked-screen behavior.
- Publish a higher signed test build to a private path and verify update check,
  download, hash validation, and system installer handoff.
- Confirm unsigned update manifests are rejected by the app.

## Known Operational Notes

- Android may still restrict long-running background behavior on aggressive
  battery-management ROMs. Users should allow background activity if automatic
  login is important.
- APK updates require the same release keystore. Losing the keystore means future
  versions cannot overwrite the installed app.
- The campus gateway uses cleartext HTTP, so the manifest keeps cleartext
  traffic enabled for `http://10.255.0.19`.
