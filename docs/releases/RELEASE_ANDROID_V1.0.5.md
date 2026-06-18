# AUST WiFi Android v1.0.5

## 更新内容

- 修复 WiFi 与 5G/移动数据同时开启时，校园网登录请求可能走移动数据导致 `Socket operation timed out` 的问题。
- 登录请求和认证网关探测现在会临时绑定到 WiFi 网络，请求结束后自动恢复系统默认网络。
- 如果系统拒绝切换登录请求到 WiFi，会给出关闭移动数据后重试的明确提示。

## 发布信息

- Version Name: `1.0.5`
- Version Code: `15`
- 平台: Android arm64-v8a
