# AUST WiFi 自动重连工具 - Linux编译指南

## 📋 概述

本文档说明如何在Linux系统上编译和运行AUST WiFi自动重连工具v2.0。

**版本**: v2.0.0  
**支持平台**: Linux (Ubuntu, Debian, Fedora, Arch Linux等)  
**开发者**: 信息安全23-1 王智杰

---

## 🖥️ 系统要求

### 硬件要求
- **处理器**: x86_64 (64位) 或 ARM64
- **内存**: 最低 2GB RAM
- **存储空间**: 200MB 可用空间

### 软件要求
- **操作系统**: Linux发行版（Ubuntu 20.04+, Debian 11+, Fedora 34+, Arch Linux等）
- **Qt版本**: Qt 6.7 或更高版本
- **编译器**: GCC 9+ 或 Clang 10+
- **构建工具**: make, qmake

---

## 📦 安装依赖

### Ubuntu/Debian系统

```bash
# 更新软件包列表
sudo apt-get update

# 安装Qt6开发包
sudo apt-get install -y \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    build-essential \
    cmake

# 安装WiFi检测工具（可选，但推荐）
sudo apt-get install -y \
    wireless-tools \
    network-manager
```

### Fedora/RHEL系统

```bash
# 安装Qt6开发包
sudo dnf install -y \
    qt6-qtbase-devel \
    qt6-qttools-devel \
    gcc-c++ \
    make \
    cmake

# 安装WiFi检测工具（可选）
sudo dnf install -y \
    wireless-tools \
    NetworkManager
```

### Arch Linux系统

```bash
# 安装Qt6开发包
sudo pacman -S --needed \
    qt6-base \
    qt6-tools \
    base-devel \
    cmake

# 安装WiFi检测工具（可选）
sudo pacman -S --needed \
    wireless_tools \
    networkmanager
```

---

## 🔨 编译步骤

### 方式一：使用编译脚本（推荐）

```bash
# 1. 进入项目目录
cd AUST-WIFI

# 2. 赋予执行权限（首次运行）
chmod +x build.sh

# 3. 运行编译脚本
./build.sh
```

编译脚本会自动：
- 检查编译环境
- 验证项目文件完整性
- 清理旧的构建文件
- 生成Makefile
- 编译项目
- 提示运行方式

### 方式二：手动编译

```bash
# 1. 进入项目目录
cd AUST-WIFI

# 2. 创建构建目录
mkdir -p build
cd build

# 3. 生成Makefile
qmake ../AUST_WIFI.pro

# 4. 编译项目（使用多核加速）
make -j$(nproc)

# 5. 运行程序
./AUST_WIFI
```

### 方式三：使用Qt Creator

1. 打开Qt Creator
2. 选择 `文件` → `打开文件或项目`
3. 选择 `AUST_WIFI.pro` 文件
4. 配置编译套件（选择Qt 6.7+）
5. 点击 `构建` → `构建项目`

---

## 🚀 运行程序

### 首次运行

```bash
# 进入构建目录
cd build

# 运行程序
./AUST_WIFI
```

### 配置说明

首次运行时会自动弹出配置对话框，需要配置：

**学生配置**：
- 学号：完整学号（如：2023305123）
- 密码：校园网登录密码
- 运营商：电信(aust) / 联通(unicom) / 移动(cmcc)

**教师配置**：
- 工号：工号（如：2021043）
- 密码：校园网登录密码
- 服务器：自动配置为jzg

### 系统托盘

程序运行后会显示在系统托盘（通知区域）：
- 🟢 **绿色**：网络连接正常
- 🔴 **红色**：网络断开，正在重连
- 🟡 **黄色**：正在检测网络状态

---

## 🔧 Linux平台特性

### WiFi SSID检测

Linux版本使用以下工具检测当前WiFi SSID：

1. **优先使用 `iwgetid`**（来自wireless-tools包）
   ```bash
   iwgetid -r
   ```

2. **备用方案 `nmcli`**（来自NetworkManager）
   ```bash
   nmcli -t -f active,ssid dev wifi
   ```

### 配置存储

Linux版本使用Qt的QSettings，配置文件位置：
- **配置文件路径**: `~/.config/AUST_WIFI/Config.conf`
- **格式**: INI格式，明文存储（密码除外）

### 开机自启动

Linux版本支持通过以下方式设置开机自启动：

#### 方式一：systemd用户服务（推荐）

```bash
# 创建服务文件
mkdir -p ~/.config/systemd/user
cat > ~/.config/systemd/user/aust-wifi.service << EOF
[Unit]
Description=AUST WiFi Auto Reconnect Tool
After=network.target

[Service]
Type=simple
ExecStart=/path/to/AUST_WIFI
Restart=on-failure

[Install]
WantedBy=default.target
EOF

# 启用服务
systemctl --user enable aust-wifi.service
systemctl --user start aust-wifi.service
```

#### 方式二：桌面启动项

```bash
# 创建.desktop文件
mkdir -p ~/.config/autostart
cat > ~/.config/autostart/aust-wifi.desktop << EOF
[Desktop Entry]
Type=Application
Name=AUST WiFi Auto Reconnect
Exec=/path/to/AUST_WIFI
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
EOF
```

---

## 🐛 故障排除

### 问题1：找不到qmake

**错误信息**：
```
qmake: command not found
```

**解决方案**：
```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev-tools

# Fedora/RHEL
sudo dnf install qt6-qtbase-devel

# Arch Linux
sudo pacman -S qt6-base
```

### 问题2：缺少Qt模块

**错误信息**：
```
Project ERROR: Unknown module(s) in QT: network widgets
```

**解决方案**：
```bash
# 确保安装了完整的Qt6开发包
sudo apt-get install qt6-base-dev qt6-tools-dev  # Ubuntu/Debian
sudo dnf install qt6-qtbase-devel qt6-qttools-devel  # Fedora
```

### 问题3：WiFi SSID检测失败

**错误信息**：
```
Linux平台未能检测到当前WiFi SSID
```

**解决方案**：
```bash
# 安装WiFi检测工具
sudo apt-get install wireless-tools network-manager  # Ubuntu/Debian
sudo dnf install wireless-tools NetworkManager  # Fedora
sudo pacman -S wireless_tools networkmanager  # Arch

# 检查工具是否可用
iwgetid -r  # 应该返回当前SSID
nmcli -t -f active,ssid dev wifi  # 备用方法
```

### 问题4：编译错误：找不到头文件

**错误信息**：
```
fatal error: QtWidgets/QApplication: No such file or directory
```

**解决方案**：
```bash
# 确保Qt6开发包已正确安装
qmake -v  # 检查Qt版本

# 如果Qt安装在非标准路径，设置环境变量
export QTDIR=/usr/lib/qt6
export PATH=$QTDIR/bin:$PATH
```

### 问题5：运行时缺少共享库

**错误信息**：
```
error while loading shared libraries: libQt6Core.so.6
```

**解决方案**：
```bash
# 安装Qt6运行时库
sudo apt-get install qt6-base-dev  # Ubuntu/Debian
sudo dnf install qt6-qtbase  # Fedora

# 或者设置库路径
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/qt6/lib:$LD_LIBRARY_PATH
```

---

## 📝 开发说明

### 项目结构

```
AUST-WIFI/
├── main.cpp              # 程序入口
├── mainwindow.h/cpp      # 主窗口
├── wifimanager.h/cpp     # 网络管理（已支持Linux）
├── config_dialog.h/cpp   # 配置对话框
├── AUST_WIFI.pro         # Qt项目文件（已更新）
├── build.sh              # Linux编译脚本
└── docs/
    └── LINUX_BUILD.md    # 本文档
```

### Linux平台特定代码

WiFi SSID检测已实现跨平台支持：

```cpp
// wifimanager.cpp
QString WiFiManager::getCurrentWifiSSID()
{
#ifdef Q_OS_LINUX
    // 使用iwgetid或nmcli检测
#elif defined(Q_OS_WIN)
    // 使用netsh检测
#elif defined(Q_OS_MACOS)
    // 使用airport检测
#endif
}
```

### 编译选项

`.pro`文件已配置为跨平台：

```pro
# 默认使用Qt网络模块，跨平台可用
DEFINES += USE_QT_NETWORK

# Windows特定配置
win32 {
    CONFIG += windows
    CONFIG -= console
    RC_ICONS = icons/app.ico
}
```

---

## 🔗 相关资源

- **项目主页**: https://github.com/Mengxun326/AUST-WIFI
- **问题反馈**: https://github.com/Mengxun326/AUST-WIFI/issues
- **Qt文档**: https://doc.qt.io/qt-6/
- **开发者联系**: 
  - 邮箱: Meng__xun@163.com
  - QQ: 3209697935

---

## 📄 许可证

本项目采用MIT许可证，详见 [LICENSE](../LICENSE) 文件。

---

**最后更新**: 2025年1月  
**版本**: v2.0.0 (Linux支持)

