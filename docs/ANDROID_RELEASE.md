# Android 发布流程

Android 端发布脚本会生成 APK 和更新清单：

```text
installer\android\AUST-WIFI-Android-版本号.apk
installer\android\update.json
```

服务器访问地址默认是：

```text
https://www.meng-xun.top/aust-wifi/android/update.json
https://www.meng-xun.top/aust-wifi/android/releases/AUST-WIFI-Android-版本号.apk
```

## 首次准备签名证书

Android 正式更新必须长期使用同一个签名证书。证书丢失后，手机会拒绝覆盖安装新版 APK，所以请离线备份 `secrets\android-release.jks` 和密码。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\new-android-signing-key.ps1
```

脚本默认生成：

```text
secrets\android-release.jks
```

发布前设置环境变量：

```powershell
$env:AUST_WIFI_ANDROID_KEYSTORE = "E:\GitHub\AUST-WIFI\secrets\android-release.jks"
$env:AUST_WIFI_ANDROID_KEY_ALIAS = "aust-wifi-android"
$env:AUST_WIFI_ANDROID_KEYSTORE_PASSWORD = "你的 keystore 密码"
$env:AUST_WIFI_ANDROID_KEY_PASSWORD = "你的 key 密码"
```

## 生成正式 APK

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-android.ps1 `
  -Notes "填写本次 Android 更新说明。"
```

脚本会执行 Qt Android Release 构建、`zipalign`、`apksigner`、签名校验、SHA-256 计算，并生成 `installer\android\update.json`。

构建脚本会自动下载并打包 KDAB `android_openssl` 中的
`libssl_3.so` / `libcrypto_3.so`。如果跳过该步骤，Android 端 HTTPS 更新
检查可能出现 `TLS initialization failed`。

如果只是验证构建和清单流程，可以生成未签名 APK：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-android.ps1 `
  -Unsigned `
  -Notes "本地流程验证。"
```

未签名 APK 不能作为正式更新安装，脚本默认也不会允许上传未签名 APK。

## 修改版本号

Android 版本号在 `android\AUST_WIFI_ANDROID.pro` 中：

```text
ANDROID_VERSION_CODE = 13
ANDROID_VERSION_NAME = 1.0.3
```

如果通过发布脚本改版本，必须同时提高 `VersionCode`：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-android.ps1 `
  -Version 1.0.0 `
  -VersionCode 10 `
  -Notes "填写本次 Android 更新说明。"
```

`VersionCode` 必须单调递增，否则 Android 会拒绝覆盖安装。

## 自动上传到宝塔服务器

服务器目录默认匹配当前 `/aust-wifi/` 静态目录：

```text
/www/wwwroot/47_121_180_250/aust-wifi/android/update.json
/www/wwwroot/47_121_180_250/aust-wifi/android/releases/AUST-WIFI-Android-版本号.apk
```

使用当前服务器 SSH 端口 `32208` 上传：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-android.ps1 `
  -Notes "填写本次 Android 更新说明。" `
  -Upload `
  -UploadUser root `
  -UploadHost 47.121.180.250 `
  -UploadPort 32208 `
  -UploadIdentityFile "C:\Users\Meng_\Downloads\47.121.180.250_id_ed25519"
```

如果服务器实际网站目录不同，可传入：

```powershell
-UploadRemoteRoot "/www/wwwroot/47_121_180_250/aust-wifi/android"
```

## 上线检查

发布后浏览器检查：

```text
https://www.meng-xun.top/aust-wifi/android/update.json
https://www.meng-xun.top/aust-wifi/android/releases/AUST-WIFI-Android-版本号.apk
```

Android 应用会读取该清单，按 `version_code` 判断是否存在新版本，下载 APK 后校验 `sha256`，然后调用系统安装器。手机首次安装站外 APK 时，需要在系统设置中允许 AUST WiFi“安装未知应用”。

正式更新必须使用同一个 release keystore 签名。debug APK、unsigned APK、或不同证书签名的 APK，都不能覆盖安装到已安装的正式版本上。
