# AUST WiFi Android v1.0.3

## 更新内容

- 修复未连接 WiFi 时仍可能发起校园网登录请求，导致 `Socket operation timed out` 的问题。
- 启动自动登录现在会等待 WiFi 连接完成；网络断开或切换时会取消正在进行的登录请求。
- Android 主界面的“立即登录”按钮在无 WiFi 时会禁用并提示先连接 WiFi。

## 发布信息

- Version Name: `1.0.3`
- Version Code: `13`
- 平台: Android arm64-v8a
