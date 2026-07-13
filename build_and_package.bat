@echo off
chcp 65001 >nul
echo ========================================
echo 悟空快递运费计算系统 - 编译和打包
echo ========================================
echo.

cd /d %~dp0

echo 步骤 1: 创建编译目录...
if exist build rmdir /s /q build
mkdir build
cd build

echo.
echo 步骤 2: 配置 CMake...
cmake ..
if %errorlevel% neq 0 (
    echo.
    echo ❌ CMake 配置失败!
    pause
    exit /b 1
)

echo.
echo 步骤 3: 编译 Release 版本...
cmake --build . --config Release -- /m
if %errorlevel% neq 0 (
    echo.
    echo ❌ 编译失败!
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✅ 编译成功!
echo ========================================
echo.

cd ..

echo 步骤 4: 创建打包目录...
if exist package_portable rmdir /s /q package_portable
mkdir package_portable

echo.
echo 步骤 5: 复制主程序...
copy /y build\Release\WukongFreight.exe package_portable\

echo.
echo 步骤 6: 复制 Qt 依赖...
for /f "delims=" %%i in ('dir /b /s "C:\Qt\6.*\msvc*\*\bin\Qt6Core.dll" 2^>nul ^| findstr /i "Qt6Core.dll"') do (
    set "qtbin=%%i"
    goto :found_qt
)
echo ⚠️ 未找到 Qt，请手动复制
goto :create_docs

:found_qt
set "qtbin=%qtbin:Qt6Core.dll=%"
echo Qt 路径：%qtbin%
copy /y "%qtbin%Qt6Core.dll" package_portable\
copy /y "%qtbin%Qt6Gui.dll" package_portable\
copy /y "%qtbin%Qt6Widgets.dll" package_portable\
copy /y "%qtbin%Qt6Concurrent.dll" package_portable\
copy /y "%qtbin%Qt6Network.dll" package_portable\

if exist "%qtbin%..\..\plugins\platforms\qwindows.dll" (
    mkdir package_portable\platforms
    copy /y "%qtbin%..\..\plugins\platforms\qwindows.dll" package_portable\platforms\
)

if exist "%qtbin%..\..\plugins\styles\qwindowsvistastyle.dll" (
    mkdir package_portable\styles
    copy /y "%qtbin%..\..\plugins\styles\qwindowsvistastyle.dll" package_portable\styles\
)

:create_docs
echo.
echo 步骤 7: 创建文档...
(
echo # 悟空快递运费计算系统 v1.0.0
echo.
echo **开发**: 杭州喵喵至家网络有限公司
echo **日期**: 2026-07-09
echo.
echo ## 快速开始
echo.
echo 1. 双击运行 WukongFreight.exe
echo 2. 导入 Excel 账单
echo 3. 点击"开始计算"
echo 4. 导出结果
echo.
echo ## 新功能
echo.
echo 在主界面点击"**规则说明**"按钮查看完整规则。
echo.
echo ## 系统要求
echo.
echo - Windows 10/11 ^(64 位^)
echo - 4GB RAM 以上
echo.
echo **Copyright © 2026 杭州喵喵至家网络有限公司**
) > package_portable\README.md

echo ✅ README.md

(
echo # 更新日志
echo.
echo ## v1.0.0 ^(2026-07-09^)
echo.
echo ### 新增
echo - 规则说明功能
echo - 完整的计算规则文档
echo.
echo ### 修复
echo - 重量段边界条件错误
echo - 计算模式逻辑混乱
echo - Python/C++ 逻辑不一致
echo - 价格倒挂问题
) > package_portable\CHANGELOG.md

echo ✅ CHANGELOG.md

echo.
echo ========================================
echo 打包完成!
echo ========================================
echo.
echo 打包目录：package_portable
echo 文件列表:
dir /b package_portable

echo.
echo 下一步:
echo   1. 手动压缩 package_portable 目录为 ZIP
echo   2. 或运行 PowerShell: Compress-Archive -Path package_portable\* -DestinationPath WukongFreight_v1.0.0.zip
echo.

pause
