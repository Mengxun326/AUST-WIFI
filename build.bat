@echo off
setlocal enabledelayedexpansion
title AUST WiFi 自动重连工具 - 编译脚本 v1.0
color 0A

echo ========================================================
echo     AUST WiFi 自动重连工具 - 编译脚本 v1.1
echo ========================================================
echo.
echo 🚀 正在开始编译安徽理工大学校园网自动重连工具...
echo    基于 Qt6/C++ 开发，支持学生和教师用户，智能重连和系统托盘功能
echo.

REM 显示编译环境信息
echo 📋 编译环境检查:
echo ========================================================

REM 检查操作系统
echo ✓ 操作系统: %OS% %PROCESSOR_ARCHITECTURE%
for /f "tokens=2 delims=[]" %%i in ('ver') do set winver=%%i
echo ✓ 系统版本: %winver%
echo.

REM 检查Qt环境
echo 🔍 检查Qt开发环境...
where qmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 错误: 未找到 qmake，Qt 环境未正确配置
    echo.
    echo 🛠️ 解决方案:
    echo    1. 下载并安装 Qt 6.7+: https://www.qt.io/download
    echo    2. 安装时选择 MinGW 编译器
    echo    3. 将 Qt 的 bin 目录添加到 PATH 环境变量
    echo       例如: C:\Qt\6.7.0\mingw_64\bin
    echo    4. 重新打开命令提示符运行此脚本
    echo.
    pause
    exit /b 1
)

REM 获取Qt版本信息
for /f "tokens=*" %%i in ('qmake -v ^| findstr "Using Qt version"') do set qtver=%%i
echo ✓ Qt环境: %qtver%

REM 检查编译器
where mingw32-make >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ 编译器: MinGW (mingw32-make)
    set compiler=mingw32-make
) else (
    where make >nul 2>&1
    if %errorlevel% equ 0 (
        echo ✓ 编译器: 通用 make
        set compiler=make
    ) else (
        echo ❌ 错误: 未找到合适的编译器 (make 或 mingw32-make)
        echo.
        echo 🛠️ 解决方案:
        echo    请确保安装了 MinGW 编译器并添加到 PATH 环境变量
        echo.
        pause
        exit /b 1
    )
)
echo.

REM 检查项目文件
echo 📁 检查项目文件结构...
set missing_files=0

if exist AUST_WIFI.pro (
    echo ✓ AUST_WIFI.pro - Qt项目文件
) else (
    echo ❌ AUST_WIFI.pro - 项目文件缺失
    set /a missing_files+=1
)

if exist main.cpp (
    echo ✓ main.cpp - 程序入口文件
) else (
    echo ❌ main.cpp - 入口文件缺失
    set /a missing_files+=1
)

if exist mainwindow.h (
    echo ✓ mainwindow.h - 主窗口头文件
) else (
    echo ❌ mainwindow.h - 头文件缺失
    set /a missing_files+=1
)

if exist mainwindow.cpp (
    echo ✓ mainwindow.cpp - 主窗口实现文件
) else (
    echo ❌ mainwindow.cpp - 实现文件缺失
    set /a missing_files+=1
)

if exist wifimanager.h (
    echo ✓ wifimanager.h - WiFi管理器头文件
) else (
    echo ❌ wifimanager.h - 头文件缺失
    set /a missing_files+=1
)

if exist wifimanager.cpp (
    echo ✓ wifimanager.cpp - WiFi管理器实现文件
) else (
    echo ❌ wifimanager.cpp - 实现文件缺失
    set /a missing_files+=1
)

if %missing_files% gtr 0 (
    echo.
    echo ❌ 检测到 %missing_files% 个关键文件缺失，无法继续编译
    pause
    exit /b 1
)
echo.

REM 检查libcurl依赖
echo 🌐 检查网络库依赖...
if exist curl\include\curl\curl.h (
    echo ✓ libcurl: 已安装，将使用 libcurl 进行网络请求
    echo   - 提供更好的网络控制和错误处理
    echo   - 支持更多网络协议和选项
) else (
    echo ⚠️ libcurl: 未安装，将使用 Qt Network 模块
    echo   - Qt Network 功能完全足够日常使用
    echo   - 如需安装 libcurl，请参考项目文档
)
    echo.

REM 检查资源文件和图标
echo 🎨 检查资源文件和图标...
set icon_count=0
set resource_count=0

if exist resources.qrc (
    echo ✓ resources.qrc - 资源配置文件
    findstr /c:"icons/app.ico" resources.qrc >nul
    if !errorlevel!==0 (
        echo ✓ app.ico - 主程序图标 (已配置)
        set /a icon_count+=1
        set /a resource_count+=1
    )
) else (
    echo ⚠️ resources.qrc - 资源文件缺失，将使用默认图标
) 

REM 检查本地图标文件
if exist icons\app.ico (
    if %resource_count%==0 (
        echo ✓ app.ico - 主程序图标 (本地文件)
        set /a icon_count+=1
    )
) else (
    if %resource_count%==0 (
        echo ⚠️ app.ico - 主程序图标缺失
    )
)

if exist icons\connected.ico (
    echo ✓ connected.ico - 连接状态图标
    set /a icon_count+=1
) else (
    echo ⚠️ connected.ico - 连接图标缺失
)

if exist icons\disconnected.ico (
    echo ✓ disconnected.ico - 断开状态图标
    set /a icon_count+=1
) else (
    echo ⚠️ disconnected.ico - 断开图标缺失
)

    echo.
echo 💡 图标状态: 检测到 %icon_count%/3 个图标文件
if %icon_count% lss 3 (
    echo    程序仍可正常运行，缺失的图标将使用系统默认样式
)
echo.

REM 清理之前的构建文件
echo 🧹 清理之前的构建文件...
if exist Makefile (
    del Makefile
    echo ✓ 清理 Makefile
)
if exist *.o (
    del *.o
    echo ✓ 清理目标文件 (*.o)
)
for %%f in (moc_*.cpp) do (
    if exist "%%f" (
        del "%%f"
        echo ✓ 清理 MOC 文件 (%%f)
    )
)
for %%f in (ui_*.h) do (
    if exist "%%f" (
        del "%%f"
        echo ✓ 清理 UI 头文件 (%%f)
    )
)
if exist AUST_WIFI.exe (
    del AUST_WIFI.exe
    echo ✓ 清理旧的可执行文件
)
echo.

REM 生成 Makefile
echo 🔧 生成构建配置...
echo 正在运行: qmake AUST_WIFI.pro
qmake AUST_WIFI.pro

if %errorlevel% neq 0 (
    echo ❌ qmake 执行失败，无法生成 Makefile
    echo.
    echo 🛠️ 可能的解决方案:
    echo    1. 检查 AUST_WIFI.pro 文件语法
    echo    2. 确保 Qt 版本兼容 (推荐 6.7+)
    echo    3. 检查项目文件路径是否正确
    echo.
    pause
    exit /b 1
)
echo ✓ Makefile 生成成功
echo.

REM 开始编译
echo 🔨 开始编译项目...
echo ========================================================
echo 正在运行: %compiler%
echo.

%compiler%

echo.
echo ========================================================

if %errorlevel% equ 0 (
    echo.
    echo 🎉 编译成功！
    echo ========================================================
    echo.
    
    if exist AUST_WIFI.exe (
        echo 📁 输出文件: AUST_WIFI.exe
        for %%f in (AUST_WIFI.exe) do echo 📏 文件大小: %%~zf 字节
        echo.
        
        echo 🚀 运行程序:
        echo    双击 AUST_WIFI.exe 或在命令行输入:
        echo    AUST_WIFI.exe
        echo.
        
        echo 📋 首次运行配置:
        echo    ┌─────────────────────────────────┐
        echo    │ 学号：输入完整学号               │
        echo    │ 密码：校园网登录密码             │
        echo    │ 运营商：                         │
        echo    │   • 电信用户 → 选择"电信"       │
        echo    │   • 联通用户 → 选择"联通"       │
        echo    │   • 移动用户 → 选择"移动"       │
        echo    │ 开机自启：建议勾选               │
        echo    └─────────────────────────────────┘
        echo.
        
        echo 📚 更多信息:
        echo    • 详细说明: README.md
        echo    • 用户手册: 用户说明书.md
        echo    • 项目地址: https://github.com/Mengxun326/AUST-WIFI
        echo.
        
        echo 🎯 功能特点:
        echo    ✓ 智能网络检测 (每秒检测)
        echo    ✓ 自动重连校园网
        echo    ✓ 系统托盘后台运行
        echo    ✓ 开机自启动支持
        echo    ✓ 多运营商支持
        echo    ✓ 安全密码加密存储
        echo.
        
        set /p run_now="🤔 是否立即运行程序？(Y/N): "
        if /i "!run_now!"=="Y" (
            echo.
            echo 🚀 正在启动程序...
            start AUST_WIFI.exe
            echo ✓ 程序已启动，请查看系统托盘
        )
    ) else (
        echo ❌ 编译成功但未找到可执行文件 AUST_WIFI.exe
        echo    这可能是编译器配置问题，请检查编译输出
    )
) else (
    echo.
    echo ❌ 编译失败！
    echo ========================================================
    echo.
    echo 🛠️ 故障排除建议:
    echo.
    echo 1. 🔍 检查编译错误信息:
    echo    • 查看上面的编译输出，寻找具体错误信息
    echo    • 通常错误信息会指明问题所在的文件和行号
    echo.
    echo 2. 🔧 常见问题解决:
    echo    • Qt 版本不兼容 → 升级到 Qt 6.7+
    echo    • 缺少依赖库 → 安装 Microsoft Visual C++ Redistributable
    echo    • 权限问题 → 以管理员身份运行此脚本
    echo    • 路径包含中文 → 将项目移动到英文路径
    echo.
         echo 3. 📚 获取帮助:
     echo    • 查看 README.md 中的故障排除部分
     echo    • 访问 GitHub Issues: github.com/Mengxun326/AUST-WIFI/issues
     echo    • 邮件联系开发者: Meng__xun@163.com
     echo    • 开发者QQ: 3209697935
     echo    • 个人博客: https://www.meng-xun.top
    echo.
    echo 4. 🧪 环境测试:
    echo    • 尝试编译一个简单的 Qt 项目验证环境
    echo    • 检查 PATH 环境变量是否正确配置
    echo    • 重新安装 Qt 开发环境
    echo.
    
    set /p retry="🤔 是否重新尝试编译？(Y/N): "
    if /i "!retry!"=="Y" (
        echo.
        echo 🔄 重新尝试编译...
        goto :retry_build
    )
    
    pause
    exit /b 1
) 

echo.
echo ========================================================
echo 🙏 感谢使用 AUST WiFi 自动重连工具！
echo    让校园网连接更智能、更便捷！
echo    现在支持全体师生！
echo.
echo 开发者: 信息安全23-1 王智杰
echo 版本: v1.1.0 (新增教师用户支持)
echo ========================================================
echo.
pause
exit /b 0

:retry_build
echo 🔄 正在重新编译...
%compiler%
goto :check_result 