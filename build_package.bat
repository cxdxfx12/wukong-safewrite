@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

echo ========================================
echo 悟空运费结算系统 - 一键编译打包
echo ========================================
echo.

set PROJECT_DIR=C:\Users\ccc\Desktop\wukong-main
set QT_DIR=E:\Qt\6.12.0\mingw_64
set MINGW_BIN=E:\Qt\Tools\mingw1310_64\bin
set NINJA_BIN=E:\Qt\Tools\Ninja
set CMAKE_BIN=E:\Qt\Tools\CMake_64\bin\cmake.exe
set BUILD_DIR=%PROJECT_DIR%\build
set PACKAGE_DIR=%PROJECT_DIR%\package

cd /d %PROJECT_DIR%

echo [1/6] 清理旧文件...
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
if exist %PACKAGE_DIR% rmdir /s /q %PACKAGE_DIR%
mkdir %BUILD_DIR%
mkdir %PACKAGE_DIR%
echo.

echo [2/6] 配置 CMake...
set PATH=%MINGW_BIN%;%NINJA_BIN%;%QT_DIR%\bin;%PATH%
cd %BUILD_DIR%
%CMAKE_BIN% .. ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_CXX_COMPILER="%MINGW_BIN%/g++.exe" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -G Ninja

if %errorlevel% neq 0 (
    echo.
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)
echo.

echo [3/6] 编译项目...
%CMAKE_BIN% --build . --parallel
if %errorlevel% neq 0 (
    echo.
    echo [错误] 编译失败
    pause
    exit /b 1
)
echo.

cd %PROJECT_DIR%
echo [4/6] 复制主程序...
copy /y "%BUILD_DIR%\悟空结算.exe" "%PACKAGE_DIR%\WukongFreight.exe" >nul
echo   OK: WukongFreight.exe
echo.

echo [5/6] 复制依赖库...
set QT_DLLS=Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Concurrent.dll Qt6Network.dll
for %%d in (%QT_DLLS%) do (
    if exist "%QT_DIR%\bin\%%d" (
        copy /y "%QT_DIR%\bin\%%d" "%PACKAGE_DIR%\" >nul
        echo   OK: %%d
    )
)

set MINGW_DLLS=libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll
for %%d in (%MINGW_DLLS%) do (
    if exist "%MINGW_BIN%\%%d" (
        copy /y "%MINGW_BIN%\%%d" "%PACKAGE_DIR%\" >nul
        echo   OK: %%d
    )
)

if exist "%QT_DIR%\plugins\platforms\qwindows.dll" (
    mkdir "%PACKAGE_DIR%\platforms"
    copy /y "%QT_DIR%\plugins\platforms\qwindows.dll" "%PACKAGE_DIR%\platforms\" >nul
    echo   OK: platforms\qwindows.dll
)

if exist "%QT_DIR%\plugins\styles" (
    mkdir "%PACKAGE_DIR%\styles"
    for %%f in ("%QT_DIR%\plugins\styles\*.dll") do (
        copy /y "%%f" "%PACKAGE_DIR%\styles\" >nul
        echo   OK: styles\%%~nxf
    )
)
echo.

echo [6/6] 创建文档和压缩包...
(
echo # 悟空运费结算系统 v1.0.1
echo.
echo ## 快速开始
echo 1. 双击运行 WukongFreight.exe
echo 2. 导入 Excel 账单
echo 3. 点击"开始计算"
echo 4. 导出结果
echo.
echo ## 系统要求
echo - Windows 10/11 ^(64位^)
echo - 4GB RAM 以上
echo.
echo 开发：杭州喵喵至家网络有限公司
echo 日期：2026-07-10
) > "%PACKAGE_DIR%\README.txt"

(
echo # 更新日志 v1.0.1
echo.
echo ## 修复
echo - 五区价格表显示格式统一
echo.
echo ## v1.0.0 修复
echo - 重量段边界条件错误
echo - 计算模式逻辑混乱
echo - Python/C++ 逻辑不一致
echo - 价格倒挂问题
echo - 线程安全问题
echo - 异常处理不足
echo - 省份标准化重复
) > "%PACKAGE_DIR%\CHANGELOG.txt"

echo   OK: README.txt
echo   OK: CHANGELOG.txt
echo.

echo   创建 ZIP 压缩包...
powershell -NoProfile -Command ^
    "$ts = Get-Date -Format 'yyyyMMdd_HHmmss';" ^
    "$zip = '%PROJECT_DIR%\WukongFreight_v1.0.1_$ts.zip';" ^
    "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath $zip -Force;" ^
    "if (Test-Path $zip) {" ^
    "    $size = (Get-Item $zip).Length / 1MB;" ^
    "    Write-Host '';" ^
    "    Write-Host '========================================' -ForegroundColor Green;" ^
    "    Write-Host '打包成功！' -ForegroundColor Green;" ^
    "    Write-Host '========================================' -ForegroundColor Green;" ^
    "    Write-Host '安装包: $zip' -ForegroundColor Cyan;" ^
    "    Write-Host ('大小: {0:N2} MB' -f $size) -ForegroundColor Cyan;" ^
    "} else {" ^
    "    Write-Host '打包失败' -ForegroundColor Red;" ^
    "}"

echo.
echo ========================================
echo 完成！
echo ========================================
echo.
echo 安装包位置: %PROJECT_DIR%\WukongFreight_v1.0.1_*.zip
echo.
pause
