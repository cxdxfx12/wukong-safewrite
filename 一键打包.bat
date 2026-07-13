@echo off
chcp 65001 >nul
title 悟空结算 - 一键打包
cd /d "%~dp0"

echo ========================================
echo   悟空运费结算系统 - 一键打包
echo ========================================
echo.

:: ===== 路径配置（E盘 Qt）=====
set QT_DIR=E:\Qt\6.12.0\mingw_64
set CMAKE_DIR=E:\Qt\Tools\CMake_64\bin
set NINJA_DIR=E:\Qt\Tools\Ninja
set MINGW_DIR=E:\Qt\Tools\mingw1310_64\bin
set PATH=%CMAKE_DIR%;%NINJA_DIR%;%MINGW_DIR%;%QT_DIR%\bin;%PATH%

:: ===== 检查 Qt 是否存在 =====
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [错误] 未找到 Qt: %QT_DIR%
    echo 请确认 Qt 安装路径
    pause
    exit /b 1
)
echo [OK] Qt 路径: %QT_DIR%

:: ===== 步骤 1: 清理旧构建 =====
echo.
echo [1/5] 清理旧构建目录...
if exist build rmdir /s /q build
mkdir build
echo [OK] 已清理

:: ===== 步骤 2: CMake 配置 =====
echo.
echo [2/5] 配置 CMake (Ninja + MinGW Release)...
cd build
cmake .. -G "Ninja" ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_COMPILER=%MINGW_DIR%\gcc.exe ^
    -DCMAKE_CXX_COMPILER=%MINGW_DIR%\g++.exe

if %errorlevel% neq 0 (
    echo.
    echo [错误] CMake 配置失败!
    pause
    exit /b 1
)
echo [OK] CMake 配置成功

:: ===== 步骤 3: 编译 =====
echo.
echo [3/5] 编译中，请稍候...
ninja

if %errorlevel% neq 0 (
    echo.
    echo [错误] 编译失败!
    pause
    exit /b 1
)
echo [OK] 编译成功

:: ===== 步骤 4: 打包 =====
echo.
echo [4/5] 打包中...
cd /d "%~dp0"

:: 清理旧打包目录
if exist package rmdir /s /q package
mkdir package

:: 复制主程序
copy /y "build\悟空结算.exe" "package\悟空结算.exe" >nul
if %errorlevel% neq 0 (
    echo [错误] 复制 exe 失败
    pause
    exit /b 1
)
echo [OK] 已复制主程序

:: 运行 windeployqt 收集 Qt 依赖
echo     运行 windeployqt 收集依赖库...
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler "package\悟空结算.exe" >nul 2>&1

:: 手动补齐 MinGW 运行时 DLL（windeployqt 有时会漏）
copy /y "%MINGW_DIR%\libgcc_s_seh-1.dll" "package\" >nul 2>&1
copy /y "%MINGW_DIR%\libstdc++-6.dll" "package\" >nul 2>&1
copy /y "%MINGW_DIR%\libwinpthread-1.dll" "package\" >nul 2>&1
echo [OK] 依赖库收集完成

:: 创建说明文件
(
echo 悟空运费结算系统 v1.0.0
echo.
echo 开发: 杭州喵喵至家网络有限公司
echo.
echo 使用方法:
echo   1. 双击运行 悟空结算.exe
echo   2. 导入 Excel 账单
echo   3. 点击"开始计算"
echo   4. 导出结果
echo.
echo 系统要求:
echo   - Windows 10/11 64位
echo   - 4GB RAM 以上
echo.
echo Copyright (c) 2026 杭州喵喵至家网络有限公司
) > "package\README.txt"

echo [OK] 已创建说明文件

:: ===== 步骤 5: 生成 ZIP =====
echo.
echo [5/5] 生成 ZIP 压缩包...
set "ZIP_NAME=悟空结算_v1.0.0_%date:~0,4%%date:~5,2%%date:~8,2%_%time:~0,2%%time:~3,2%.zip"
set "ZIP_NAME=%ZIP_NAME: =0%"

powershell -NoProfile -Command "Compress-Archive -Path '%~dp0package\*' -DestinationPath '%~dp0%ZIP_NAME%' -Force"

if exist "%ZIP_NAME%" (
    echo [OK] ZIP 已生成: %ZIP_NAME%
) else (
    echo [警告] ZIP 生成失败，可手动压缩 package 目录
)

:: ===== 完成 =====
echo.
echo ========================================
echo   打包完成!
echo ========================================
echo.
echo   打包目录: %~dp0package\
echo   压缩包:   %~dp0%ZIP_NAME%
echo.
echo   文件列表:
dir /b "package\"
echo.
pause
