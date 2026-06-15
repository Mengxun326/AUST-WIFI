# AUST WiFi Android v1.0.1

## Fixes

- Bundles Android OpenSSL runtime libraries through the Qt/KDAB
  `android_openssl` package so HTTPS update checks can initialize TLS on
  devices that do not provide a compatible Qt TLS backend.
- Keeps the Android update manifest and APK installation flow unchanged.

## Release Artifacts

```text
installer\android\AUST-WIFI-Android-1.0.1.apk
installer\android\update.json
```

Published URLs:

```text
https://www.meng-xun.top/aust-wifi/android/releases/AUST-WIFI-Android-1.0.1.apk
https://www.meng-xun.top/aust-wifi/android/update.json
```

## Upgrade Note

Users already running Android v1.0.0 may need to install v1.0.1 manually once,
because v1.0.0 can fail before it downloads the update manifest. Future updates
from v1.0.1 should work through the in-app updater.
