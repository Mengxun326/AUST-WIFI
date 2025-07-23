@echo off
setlocal enabledelayedexpansion
echo 正在编译 AUST WiFi 自动重连工具...
echo.

REM 检查Qt环境
where qmake >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: 未找到 qmake，请确保已安装 Qt 并添加到 PATH 环境变量
    echo.
    echo 解决方案:
    echo 1. 下载并安装 Qt: https://www.qt.io/download
    echo 2. 将 Qt 的 bin 目录添加到 PATH 环境变量
    echo.
    pause
    exit /b 1
)

echo Qt 环境检查通过 ✓
echo.

REM 检查libcurl
if exist curl\include\curl\curl.h (
    echo 检测到 libcurl，将使用 libcurl 进行网络请求 ✓
) else (
    echo 未检测到 libcurl，将使用 Qt 网络功能 ✓
    echo 注意: 如果需要更好的网络控制，可以安装 libcurl
    echo 安装方法请参考 安装指南.md
    echo.
)

REM 检查图标文件
echo 检查图标文件...
set icon_count=0
set resource_count=0

REM 检查资源文件中的图标引用
if exist resources.qrc (
    findstr /c:"icons/app.ico" resources.qrc >nul
    if !errorlevel!==0 (
        echo - app.ico ✓ (资源文件)
        set /a icon_count+=1
        set /a resource_count+=1
    )
) 

REM 检查本地图标文件
if exist icons\app.ico (
    if %resource_count%==0 (
        echo - app.ico ✓ (本地文件)
        set /a icon_count+=1
    )
) else (
    if %resource_count%==0 (
        echo - app.ico ✗ (使用系统默认图标)
    )
)

if exist icons\connected.ico (
    echo - connected.ico ✓  
    set /a icon_count+=1
) else (
    echo - connected.ico ✗ (使用系统默认图标)
)

if exist icons\disconnected.ico (
    echo - disconnected.ico ✓
    set /a icon_count+=1
) else (
    echo - disconnected.ico ✗ (使用系统默认图标)
)

if exist icons\warning.ico (
    echo - warning.ico ✓
    set /a icon_count+=1
) else (
    echo - warning.ico ✗ (使用系统默认图标)
)

if %icon_count%==0 (
    echo.
    echo 💡 提示: 没有检测到自定义图标文件
    echo    程序会使用系统内置图标，功能完全正常
    echo    如需自定义图标，请运行: download_icons.bat
) else (
    echo.
    echo 检测到 %icon_count%/4 个自定义图标文件 ✓
)
echo.

REM 清理之前的构建文件
echo 清理之前的构建文件...
if exist Makefile del Makefile
if exist moc_*.cpp del moc_*.cpp
if exist ui_*.h del ui_*.h
if exist *.o del *.o
if exist AUST_WIFI.exe del AUST_WIFI.exe

REM 生成 Makefile
echo 生成 Makefile...
qmake AUST_WIFI.pro

if %errorlevel% neq 0 (
    echo 错误: qmake 执行失败
    pause
    exit /b 1
)

REM 编译项目
echo 编译项目...
make

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo 编译成功！✓
    echo ========================================
    echo.
    echo 可执行文件: AUST_WIFI.exe
    echo.
    if exist AUST_WIFI.exe (
        echo 文件大小: 
        dir AUST_WIFI.exe | find "AUST_WIFI.exe"
        echo.
        echo 运行程序:
        echo AUST_WIFI.exe
        echo.
        echo 首次运行会弹出配置对话框，请填写:
        echo - 学号
        echo - 密码  
        echo - 运营商 (电信/联通/移动)
        echo.
    )
    echo.
    echo 如果遇到问题，请参考 安装指南.md
    echo.
    pause
) else (
    echo.
    echo ========================================
    echo 编译失败！✗
    echo ========================================
    echo.
    echo 可能的解决方案:
    echo 1. 检查 Qt 版本是否兼容
    echo 2. 确保所有依赖项正确安装
    echo 3. 参考 安装指南.md 中的故障排除部分
    echo.
    pause
    exit /b 1
) 