# 悟空快递运费计算系统 - 便携版打包脚本
# 日期：2026-07-09
# 功能：创建便携版安装包（包含所有依赖）

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "悟空快递运费计算系统 - 便携版打包" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 设置目录
$projectDir = "C:\Users\ccc\Desktop\wukong-main"
$buildDir = "$projectDir\build"
$packageDir = "$projectDir\package_portable"
$releaseDir = "$buildDir\Release"

Write-Host "项目目录：$projectDir" -ForegroundColor Yellow
Write-Host "打包目录：$packageDir" -ForegroundColor Yellow
Write-Host ""

# 1. 编译项目
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 1: 编译项目" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if (-not (Test-Path "$releaseDir\WukongFreight.exe")) {
    Write-Host "未找到编译文件，开始编译..." -ForegroundColor Yellow
    
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    
    mkdir $buildDir | Out-Null
    Push-Location $buildDir
    
    cmake .. -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) { exit 1 }
    
    cmake --build . --config Release -- /m
    if ($LASTEXITCODE -ne 0) { exit 1 }
    
    Pop-Location
    Write-Host "✅ 编译完成" -ForegroundColor Green
} else {
    Write-Host "✅ 使用已有编译文件" -ForegroundColor Green
}

Write-Host ""

# 2. 创建打包目录
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 2: 创建打包目录" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if (Test-Path $packageDir) {
    Remove-Item -Recurse -Force $packageDir
}

mkdir $packageDir | Out-Null

Write-Host "✅ 打包目录已创建" -ForegroundColor Green
Write-Host ""

# 3. 复制主程序
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 3: 复制主程序" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Copy-Item "$releaseDir\WukongFreight.exe" "$packageDir\" -Force
Write-Host "✅ WukongFreight.exe" -ForegroundColor Green

# 4. 复制 Qt 和依赖库
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 4: 复制依赖库" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 获取 Visual Studio 和 Qt 路径
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -property installationPath
if ($vsPath) {
    $msbuildPath = "$vsPath\MSBuild\Current\Bin"
}

# 查找 Qt6 路径
$qtPath = Get-ChildItem -Path "C:\Qt" -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "6.*" } | Select-Object -First 1
if ($qtPath) {
    $qtBinPath = "$($qtPath.FullName)\msvc*\*\bin" | Get-ChildItem -ErrorAction SilentlyContinue | Select-Object -First 1 | Split-Path
}

if (-not $qtBinPath) {
    Write-Host "⚠️ 未找到 Qt6，尝试使用环境变量..." -ForegroundColor Yellow
    $qtBinPath = $env:Qt6Dir
}

Write-Host "Qt 路径：$qtBinPath" -ForegroundColor Yellow

# 复制必要的 DLL
$dlls = @(
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Concurrent.dll",
    "Qt6Network.dll"
)

foreach ($dll in $dlls) {
    $src = "$qtBinPath\$dll"
    if (Test-Path $src) {
        Copy-Item $src "$packageDir\" -Force
        Write-Host "✅ $dll" -ForegroundColor Green
    } else {
        Write-Host "⚠️ 未找到：$dll" -ForegroundColor Yellow
    }
}

# 复制平台插件
$platformsDir = "$qtBinPath\..\..\plugins\platforms"
if (Test-Path "$platformsDir\qwindows.dll") {
    mkdir "$packageDir\platforms" | Out-Null
    Copy-Item "$platformsDir\qwindows.dll" "$packageDir\platforms\" -Force
    Write-Host "✅ platforms\qwindows.dll" -ForegroundColor Green
}

# 复制样式插件
$stylesDir = "$qtBinPath\..\..\plugins\styles"
if (Test-Path "$stylesDir\qwindowsvistastyle.dll") {
    mkdir "$packageDir\styles" | Out-Null
    Copy-Item "$stylesDir\qwindowsvistastyle.dll" "$packageDir\styles\" -Force
    Write-Host "✅ styles\qwindowsvistastyle.dll" -ForegroundColor Green
}

# 复制 OpenSSL（如果有）
$sslLibs = @("libcrypto-3-x64.dll", "libssl-3-x64.dll")
foreach ($lib in $sslLibs) {
    $src = "$qtBinPath\$lib"
    if (Test-Path $src) {
        Copy-Item $src "$packageDir\" -Force
        Write-Host "✅ $lib" -ForegroundColor Green
    }
}

# 复制 VC++ 运行时
Write-Host ""
Write-Host "复制 VC++ 运行时..." -ForegroundColor Cyan
if ($vsPath) {
    $vcredistDir = "$vsPath\VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT"
    $vcredistFiles = Get-ChildItem -Path $vcredistDir -Filter "*.dll" -ErrorAction SilentlyContinue
    if ($vcredistFiles) {
        foreach ($file in $vcredistFiles) {
            Copy-Item $file.FullName "$packageDir\" -Force
            Write-Host "✅ $($file.Name)" -ForegroundColor Green
        }
    }
}

Write-Host ""

# 5. 创建文档
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 5: 创建文档" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$readme = @"
# 悟空快递运费计算系统 v1.0.0

**开发**: 杭州喵喵至家网络有限公司  
**日期**: 2026-07-09

---

## 🚀 快速开始

1. 双击运行 `WukongFreight.exe`
2. 导入 Excel 账单
3. 点击"开始计算"
4. 导出结果

---

## ✨ 新功能

### 规则说明

在主界面点击"**规则说明**"按钮，可以查看：
- 4 种计算模式详解
- 6 区域完整价格表
- 计算公式和案例
- 7 个常见问题解答

---

## 🔧 系统要求

- Windows 10/11 (64 位)
- 4GB RAM 以上
- 无需安装其他运行库（已包含所有依赖）

---

## 📦 包内容

- `WukongFreight.exe` - 主程序
- `Qt6*.dll` - Qt 运行时库
- `*.dll` - VC++ 运行时
- `platforms\` - 平台插件
- `styles\` - 样式插件
- `README.md` - 本文件
- `CHANGELOG.md` - 更新日志

---

## 📞 支持

邮箱：support@example.com  
时间：周一至周五 9:00-18:00

---

**Copyright © 2026 杭州喵喵至家网络有限公司**
"@

$readme | Out-File -FilePath "$packageDir\README.md" -Encoding UTF8
Write-Host "✅ README.md" -ForegroundColor Green

$changelog = @"
# 更新日志

## v1.0.0 (2026-07-09)

### 新增
- ✨ 规则说明功能（主界面按钮）
- ✨ 完整的计算规则文档
- ✨ 常见问题解答

### 修复
- 🐛 重量段边界条件错误
- 🐛 计算模式逻辑混乱
- 🐛 Python/C++ 逻辑不一致
- 🐛 价格倒挂问题（3kg 断崖）

### 改进
- 📊 计算精度提升
- 📊 代码质量优化
- 📊 用户体验改进

---

详细文档请参考项目目录中的 COMPLETE_FIX_REPORT.md
"@

$changelog | Out-File -FilePath "$packageDir\CHANGELOG.md" -Encoding UTF8
Write-Host "✅ CHANGELOG.md" -ForegroundColor Green

Write-Host ""

# 6. 创建压缩包
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 6: 创建压缩包" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$zipName = "WukongFreight_v1.0.0_Portable_$timestamp.zip"
$zipPath = "$projectDir\$zipName"

Write-Host "创建 ZIP 压缩包..." -ForegroundColor Cyan
Compress-Archive -Path "$packageDir\*" -DestinationPath $zipPath -Force

if (Test-Path $zipPath) {
    $size = (Get-Item $zipPath).Length / 1MB
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "✅ 打包成功！" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "安装包：" -ForegroundColor Yellow
    Write-Host "  $zipPath" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "大小：{0:N2} MB" -f $size -ForegroundColor Cyan
    Write-Host ""
    Write-Host "内容：" -ForegroundColor Yellow
    Get-ChildItem $packageDir -File | ForEach-Object {
        Write-Host "  - $($_.Name)" -ForegroundColor White
    }
    Write-Host ""
    Write-Host "下一步：" -ForegroundColor Yellow
    Write-Host "  1. 解压到任意目录" -ForegroundColor White
    Write-Host "  2. 运行 WukongFreight.exe" -ForegroundColor White
    Write-Host "  3. 测试所有功能" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "❌ 打包失败！" -ForegroundColor Red
}

Set-Location $projectDir
