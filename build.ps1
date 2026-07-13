# 悟空快递运费计算系统 - 快速编译脚本
# 日期：2026-07-09

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "悟空快递运费计算系统 - 编译脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 设置项目目录
$projectDir = "C:\Users\ccc\Desktop\wukong-main"
$buildDir = "$projectDir\build"

Write-Host "项目目录：$projectDir" -ForegroundColor Yellow
Write-Host ""

# 检查项目文件是否存在
if (-not (Test-Path "$projectDir\CMakeLists.txt")) {
    Write-Host "错误：找不到 CMakeLists.txt" -ForegroundColor Red
    Write-Host "请确认项目目录正确" -ForegroundColor Yellow
    exit 1
}

# 清理旧的 build 目录
if (Test-Path $buildDir) {
    Write-Host "清理旧的 build 目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# 创建新的 build 目录
Write-Host "创建 build 目录..." -ForegroundColor Green
mkdir $buildDir | Out-Null

# 进入 build 目录
Set-Location $buildDir

# 配置 CMake
Write-Host ""
Write-Host "配置 CMake..." -ForegroundColor Cyan
cmake ..

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "❌ CMake 配置失败！" -ForegroundColor Red
    Write-Host "请检查错误信息 above" -ForegroundColor Yellow
    exit 1
}

# 编译 Release 版本
Write-Host ""
Write-Host "编译 Release 版本..." -ForegroundColor Cyan
cmake --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "❌ 编译失败！" -ForegroundColor Red
    Write-Host "请检查错误信息 above" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "✅ 编译成功！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "可执行文件位置：" -ForegroundColor Yellow
Write-Host "  $buildDir\Release\WukongFreight.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "运行程序：" -ForegroundColor Yellow
Write-Host "  .\Release\WukongFreight.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "测试清单：" -ForegroundColor Yellow
Write-Host "  1. 主界面是否显示'规则说明'按钮" -ForegroundColor White
Write-Host "  2. 点击按钮是否能打开规则说明对话框" -ForegroundColor White
Write-Host "  3. 规则内容是否正确显示" -ForegroundColor White
Write-Host "  4. 原有功能（导入、计算、导出）是否正常" -ForegroundColor White
Write-Host ""

# 返回项目目录
Set-Location $projectDir
