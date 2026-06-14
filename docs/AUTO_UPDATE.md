# AUST WiFi 自动更新发布流程

当前方案采用“应用内检查更新 + 签名更新清单 + 下载新版 Inno Setup 安装包 + SHA-256 校验 + 启动安装程序”的方式。应用每次启动都会自动检查一次更新，网络较早可用时会提前触发；用户也可以在托盘菜单中点击“检查更新”。

## 服务器目录

当前站点会把 `meng-xun.top` 跳转到 `www.meng-xun.top`，因此更新地址统一使用 `www.meng-xun.top`。在服务器上准备一个 HTTPS 静态目录：

```text
https://www.meng-xun.top/aust-wifi/update.json
https://www.meng-xun.top/aust-wifi/releases/AUST-WIFI-Setup-4.0.0.exe
```

如果使用 Nginx，可以把服务器上的某个目录映射到 `/aust-wifi/`。客户端会拒绝非 HTTPS 的安装包地址。

Nginx 示例：

```nginx
location ^~ /aust-wifi/ {
    alias /var/www/aust-wifi/;
    default_type application/octet-stream;
    add_header Cache-Control "no-cache";
}

location = /aust-wifi/update.json {
    alias /var/www/aust-wifi/update.json;
    default_type application/json;
    add_header Cache-Control "no-cache";
}
```

对应服务器文件路径：

```text
/var/www/aust-wifi/update.json
/var/www/aust-wifi/releases/AUST-WIFI-Setup-4.0.0.exe
```

## 更新清单

`update.json` 示例：

```json
{
  "latest": "4.0.0",
  "min_supported": "3.0.0",
  "url": "https://www.meng-xun.top/aust-wifi/releases/AUST-WIFI-Setup-4.0.0.exe",
  "sha256": "替换为安装包的 SHA-256",
  "size": 10485760,
  "published_at": "2026-06-14",
  "notes": "升级可信更新、DPAPI 密码保护和诊断能力。",
  "force": false,
  "manifest_version": 2,
  "signature_algorithm": "rsa-sha256-pkcs1-v1_5",
  "payload": "发布脚本自动生成",
  "signature": "发布脚本自动生成"
}
```

字段说明：

- `latest`：服务器最新版本，必须大于程序内的 `APP_VERSION` 才会提示更新。
- `min_supported`：最低支持版本，当前主要用于后续兼容策略预留。
- `url`：安装包 HTTPS 地址。
- `sha256`：安装包 SHA-256，必须填写，否则客户端不会下载。
- `size`：安装包字节数，用于显示下载大小。
- `notes`：更新说明。
- `force`：重要更新标记，当前会在提示文案中强调，但仍由用户确认安装。
- `payload`：上述更新字段的紧凑 JSON 后再 Base64 编码，V4 客户端会优先校验并解析它。
- `signature`：对 payload 原始 JSON 字节的 RSA-SHA256 签名。V4 客户端默认拒绝未签名清单。

建议使用无 BOM 的 UTF-8 保存 `update.json`。项目发布脚本会自动生成无 BOM 文件。

## 签名密钥

首次发布 V4 前生成本机签名私钥：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\new-update-signing-key.ps1
```

脚本会生成 `secrets\update-signing-private.xml`。该目录已加入 `.gitignore`，请不要提交私钥，并建议离线备份。客户端内置的是对应公钥；如果私钥丢失，后续版本需要更换公钥并重新发布客户端。

## 一键发布

推荐使用项目内的发布脚本：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-windows.ps1 -Clean -Notes "填写本次更新说明。"
```

脚本会完成版本号同步、Qt Release 编译、`windeployqt`、Inno Setup 打包、SHA-256 计算、更新清单签名，并在 `installer\update.json` 中生成可直接上传的更新清单。

## 自动上传到宝塔服务器

发布脚本支持通过 SSH/SCP 自动上传。默认上传位置匹配当前宝塔 Nginx 配置：

```text
/www/wwwroot/47_121_180_250/aust-wifi/update.json
/www/wwwroot/47_121_180_250/aust-wifi/releases/AUST-WIFI-Setup-版本号.exe
```

第一次使用前，建议在 Windows 本机生成一个专门用于发布的 SSH 密钥，不要把服务器密码写进脚本：

```powershell
ssh-keygen -t ed25519 -f "$env:USERPROFILE\.ssh\aust_wifi_deploy" -C "aust-wifi-release"
Get-Content "$env:USERPROFILE\.ssh\aust_wifi_deploy.pub"
```

然后在宝塔面板中打开“终端”，把上一步输出的公钥加入服务器账号的 `authorized_keys`。如果使用 `root` 账号，可以执行：

```bash
mkdir -p ~/.ssh
chmod 700 ~/.ssh
echo '这里粘贴 aust_wifi_deploy.pub 的完整内容' >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
mkdir -p /www/wwwroot/47_121_180_250/aust-wifi/releases
```

如果你已经有服务器 root 私钥，也可以把私钥放到 Windows 的 `.ssh` 目录，例如：

```text
C:\Users\Meng_\.ssh\aust_wifi_root_ed25519
```

当前服务器 SSH 端口为 `32208`。回到 Windows，先测试 SSH 是否能免密登录：

```powershell
ssh -p 32208 -i "$env:USERPROFILE\.ssh\aust_wifi_root_ed25519" root@47.121.180.250 "ls -ld /www/wwwroot/47_121_180_250/aust-wifi"
```

测试通过后，发布并自动上传：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-windows.ps1 `
  -Clean `
  -Notes "填写本次更新说明。" `
  -Upload `
  -UploadUser root `
  -UploadHost 47.121.180.250 `
  -UploadPort 32208 `
  -UploadIdentityFile "$env:USERPROFILE\.ssh\aust_wifi_root_ed25519"
```

如果服务器网站目录不是 `/www/wwwroot/47_121_180_250/aust-wifi`，可以通过 `-UploadRemoteRoot` 覆盖。脚本默认 SSH 端口已经设置为 `32208`，如果服务器端口后续变化，可以通过 `-UploadPort` 覆盖。使用密码登录也可以省略 `-UploadIdentityFile`，但脚本会在上传阶段要求输入 SSH 密码，不适合无人值守发布。

如果 Qt 或 Inno Setup 安装目录不同，可以传入参数覆盖：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\release-windows.ps1 `
  -QtBin "E:\QT\6.11.1\mingw_64\bin" `
  -MingwBin "E:\QT\Tools\mingw1310_64\bin" `
  -InnoCompiler "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" `
  -Notes "修复自动重连稳定性。"
```

生成后上传：

```text
installer\AUST-WIFI-Setup-版本号.exe -> /aust-wifi/releases/AUST-WIFI-Setup-版本号.exe
installer\update.json -> /aust-wifi/update.json
```

如果已经配置 `-Upload`，这一步会由脚本自动完成。

## 手工发布

如果不使用脚本，可以按下面步骤执行：

1. 修改 `APP_VERSION.txt` 中的版本号，例如 `4.0.0`。
2. 使用 Qt Creator 的 Release 配置构建项目。
3. 创建部署目录并复制可执行文件。

```powershell
New-Item -ItemType Directory -Force dist\AUST_WIFI
Copy-Item build\...\release\AUST_WIFI.exe dist\AUST_WIFI\
```

4. 运行 `windeployqt`。

```powershell
windeployqt --release --compiler-runtime dist\AUST_WIFI\AUST_WIFI.exe
```

5. 使用 Inno Setup 编译安装包。

```powershell
iscc setup.iss
```

6. 计算 SHA-256。

```powershell
Get-FileHash installer\AUST-WIFI-Setup-4.0.0.exe -Algorithm SHA256
```

7. 上传安装包到服务器的 `aust-wifi/releases/` 目录，并更新 `update.json`。

## 版本号同步

发布脚本会自动同步这些位置：

- `APP_VERSION.txt`
- `app_config.h` 中的 `APP_VERSION`
- `setup.iss` 中的 `MyAppVersion`
- `update.json` 中的 `latest`
- 安装包文件名中的版本号

如果四处版本不一致，客户端可能无法正确判断更新。

## 上线检查

发布后确认下面两个地址能在浏览器中访问：

```text
https://www.meng-xun.top/aust-wifi/update.json
https://www.meng-xun.top/aust-wifi/releases/AUST-WIFI-Setup-版本号.exe
```

然后在旧版本客户端中点击托盘菜单的“检查更新”。如果 `latest` 大于旧客户端的 `APP_VERSION`，应该弹出更新提示。
