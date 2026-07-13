@echo off
chcp 65001 >nul
cd /d %~dp0

echo ========================================
echo 悟空快递运费计算系统 - 编译
echo ========================================
echo.

echo Step 1: Create build directory...
if exist build rmdir /s /q build
mkdir build
cd build

echo.
echo Step 2: Configure CMake...
cmake ..
if %errorlevel% neq 0 (
    echo CMake failed!
    pause
    exit /b 1
)

echo.
echo Step 3: Build Release...
cmake --build . --config Release -- /m
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build SUCCESS!
echo ========================================
echo.

cd ..
echo Step 4: Create package...
if exist package rmdir /s /q package
mkdir package

copy /y build\Release\WukongFreight.exe package\

echo.
echo Package created in: %~dp0package
echo.
pause
