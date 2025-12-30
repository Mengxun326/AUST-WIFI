#!/bin/bash

# AUST WiFi 自动重连工具 - Linux编译脚本
# 版本: v2.0.0
# 开发者: 信息安全23-1 王智杰

set -e  # 遇到错误立即退出

echo "========================================================"
echo "    AUST WiFi 自动重连工具 - Linux编译脚本 v2.0"
echo "========================================================"
echo ""
echo "🚀 正在开始编译安徽理工大学校园网自动重连工具..."
echo "    基于 Qt6/C++ 开发，支持学生和教师用户，智能重连功能"
echo "    现已支持Linux平台！"
echo ""

# 显示编译环境信息
echo "📋 编译环境检查:"
echo "========================================================"

# 检查操作系统
if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "✓ 操作系统: $NAME $VERSION"
else
    echo "✓ 操作系统: $(uname -s) $(uname -r)"
fi
echo "✓ 系统架构: $(uname -m)"
echo ""

# 检查Qt环境
echo "🔍 检查Qt开发环境..."
if ! command -v qmake &> /dev/null; then
    echo "❌ 错误: 未找到 qmake，Qt 环境未正确配置"
    echo ""
    echo "🛠️ 解决方案:"
    echo "    Ubuntu/Debian:"
    echo "      sudo apt-get update"
    echo "      sudo apt-get install qt6-base-dev qt6-base-dev qt6-tools-dev qt6-tools-dev-tools"
    echo ""
    echo "    Fedora/RHEL:"
    echo "      sudo dnf install qt6-qtbase-devel qt6-qttools-devel"
    echo ""
    echo "    Arch Linux:"
    echo "      sudo pacman -S qt6-base qt6-tools"
    echo ""
    echo "    或者从Qt官网下载: https://www.qt.io/download"
    echo ""
    exit 1
fi

# 获取Qt版本信息
QT_VERSION=$(qmake -v 2>&1 | grep -oP 'Using Qt version \K[0-9.]+' || echo "未知")
echo "✓ Qt环境: Qt $QT_VERSION"

# 检查编译器
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    echo "✓ 编译器: $GCC_VERSION"
elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1)
    echo "✓ 编译器: $CLANG_VERSION"
else
    echo "❌ 错误: 未找到合适的C++编译器 (g++ 或 clang++)"
    echo ""
    echo "🛠️ 解决方案:"
    echo "    Ubuntu/Debian: sudo apt-get install build-essential"
    echo "    Fedora/RHEL: sudo dnf groupinstall 'Development Tools'"
    echo "    Arch Linux: sudo pacman -S base-devel"
    echo ""
    exit 1
fi
echo ""

# 检查项目文件
echo "📁 检查项目文件结构..."
MISSING_FILES=0

check_file() {
    if [ -f "$1" ]; then
        echo "✓ $1 - 存在"
    else
        echo "❌ $1 - 文件缺失"
        MISSING_FILES=$((MISSING_FILES + 1))
    fi
}

check_file "AUST_WIFI.pro"
check_file "main.cpp"
check_file "mainwindow.h"
check_file "mainwindow.cpp"
check_file "wifimanager.h"
check_file "wifimanager.cpp"
check_file "config_dialog.h"
check_file "config_dialog.cpp"

if [ $MISSING_FILES -gt 0 ]; then
    echo ""
    echo "❌ 检测到 $MISSING_FILES 个关键文件缺失，无法继续编译"
    exit 1
fi
echo ""

# 检查Linux WiFi工具
echo "🌐 检查Linux WiFi工具..."
if command -v iwgetid &> /dev/null || command -v nmcli &> /dev/null; then
    echo "✓ WiFi检测工具: 已安装"
    if command -v iwgetid &> /dev/null; then
        echo "  - iwgetid: 可用"
    fi
    if command -v nmcli &> /dev/null; then
        echo "  - nmcli: 可用"
    fi
else
    echo "⚠️ WiFi检测工具: 未安装（可选）"
    echo "  安装命令:"
    echo "    Ubuntu/Debian: sudo apt-get install wireless-tools network-manager"
    echo "    Fedora/RHEL: sudo dnf install wireless-tools NetworkManager"
    echo "    Arch Linux: sudo pacman -S wireless_tools networkmanager"
fi
echo ""

# 检查资源文件和图标
echo "🎨 检查资源文件和图标..."
ICON_COUNT=0

if [ -f "resources.qrc" ]; then
    echo "✓ resources.qrc - 资源配置文件"
    if grep -q "icons/app.ico" resources.qrc 2>/dev/null; then
        echo "✓ app.ico - 主程序图标 (已配置)"
        ICON_COUNT=$((ICON_COUNT + 1))
    fi
fi

if [ -f "icons/app.ico" ]; then
    if [ $ICON_COUNT -eq 0 ]; then
        echo "✓ app.ico - 主程序图标 (本地文件)"
        ICON_COUNT=$((ICON_COUNT + 1))
    fi
fi

echo "💡 图标状态: 检测到 $ICON_COUNT/1 个图标文件"
if [ $ICON_COUNT -lt 1 ]; then
    echo "    程序仍可正常运行，将使用系统默认图标"
fi
echo ""

# 清理之前的构建文件
echo "🧹 清理之前的构建文件..."
rm -f Makefile
rm -f *.o
rm -f moc_*.cpp
rm -f ui_*.h
rm -f qrc_*.cpp
rm -f AUST_WIFI
rm -rf build
echo "✓ 清理完成"
echo ""

# 创建构建目录
echo "📂 创建构建目录..."
mkdir -p build
cd build
echo "✓ 构建目录已创建"
echo ""

# 生成 Makefile
echo "🔧 生成构建配置..."
echo "正在运行: qmake ../AUST_WIFI.pro"
qmake ../AUST_WIFI.pro

if [ $? -ne 0 ]; then
    echo "❌ qmake 执行失败，无法生成 Makefile"
    echo ""
    echo "🛠️ 可能的解决方案:"
    echo "    1. 检查 AUST_WIFI.pro 文件语法"
    echo "    2. 确保 Qt 版本兼容 (推荐 6.7+)"
    echo "    3. 检查项目文件路径是否正确"
    echo ""
    exit 1
fi
echo "✓ Makefile 生成成功"
echo ""

# 开始编译
echo "🔨 开始编译项目..."
echo "========================================================"
echo "正在运行: make -j$(nproc)"
echo ""

make -j$(nproc)

echo ""
echo "========================================================"

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 编译成功！"
    echo "========================================================"
    echo ""
    
    if [ -f "AUST_WIFI" ]; then
        FILE_SIZE=$(du -h AUST_WIFI | cut -f1)
        echo "📁 输出文件: build/AUST_WIFI"
        echo "📏 文件大小: $FILE_SIZE"
        echo ""
        
        echo "🚀 运行程序:"
        echo "    cd build"
        echo "    ./AUST_WIFI"
        echo ""
        
        echo "📋 首次运行配置:"
        echo "    ┌─────────────────────────────────┐"
        echo "    │ 学生配置：                       │"
        echo "    │   学号：输入完整学号             │"
        echo "    │   密码：校园网登录密码           │"
        echo "    │   运营商：电信/联通/移动         │"
        echo "    │                                 │"
        echo "    │ 教师配置：                       │"
        echo "    │   工号：输入工号                │"
        echo "    │   密码：校园网登录密码           │"
        echo "    │   服务器：自动配置               │"
        echo "    │                                 │"
        echo "    │ 开机自启：建议勾选               │"
        echo "    └─────────────────────────────────┘"
        echo ""
        
        echo "📚 更多信息:"
        echo "    • 详细说明: README.md"
        echo "    • 用户手册: docs/用户说明书.md"
        echo "    • 项目地址: https://github.com/Mengxun326/AUST-WIFI"
        echo ""
        
        echo "🎯 功能特点:"
        echo "    ✓ 智能网络检测 (每秒检测)"
        echo "    ✓ 自动重连校园网"
        echo "    ✓ 系统托盘后台运行"
        echo "    ✓ 开机自启动支持"
        echo "    ✓ 多运营商支持"
        echo "    ✓ 学生和教师双配置"
        echo "    ✓ Linux平台支持"
        echo ""
        
        read -p "🤔 是否立即运行程序？(Y/n): " RUN_NOW
        if [[ "$RUN_NOW" =~ ^[Yy]$ ]] || [ -z "$RUN_NOW" ]; then
            echo ""
            echo "🚀 正在启动程序..."
            ./AUST_WIFI &
            echo "✓ 程序已启动，请查看系统托盘或窗口"
        fi
    else
        echo "❌ 编译成功但未找到可执行文件 AUST_WIFI"
        echo "    这可能是编译器配置问题，请检查编译输出"
    fi
else
    echo ""
    echo "❌ 编译失败！"
    echo "========================================================"
    echo ""
    echo "🛠️ 故障排除建议:"
    echo ""
    echo "1. 🔍 检查编译错误信息:"
    echo "    • 查看上面的编译输出，寻找具体错误信息"
    echo "    • 通常错误信息会指明问题所在的文件和行号"
    echo ""
    echo "2. 🔧 常见问题解决:"
    echo "    • Qt 版本不兼容 → 升级到 Qt 6.7+"
    echo "    • 缺少依赖库 → 安装Qt开发包"
    echo "    • 权限问题 → 检查文件权限"
    echo "    • 路径包含中文 → 将项目移动到英文路径"
    echo ""
    echo "3. 📚 获取帮助:"
    echo "    • 查看 README.md 中的故障排除部分"
    echo "    • 访问 GitHub Issues: github.com/Mengxun326/AUST-WIFI/issues"
    echo "    • 邮件联系开发者: Meng__xun@163.com"
    echo "    • 开发者QQ: 3209697935"
    echo ""
    exit 1
fi

cd ..

echo ""
echo "========================================================"
echo "🙏 感谢使用 AUST WiFi 自动重连工具！"
echo "    让校园网连接更智能、更便捷！"
echo "    现在支持Linux平台！"
echo ""
echo "开发者: 信息安全23-1 王智杰"
echo "版本: v2.0.0 (新增Linux平台支持)"
echo "========================================================"
echo ""

