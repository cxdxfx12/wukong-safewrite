# 悟空快递运费计算系统 - 快速打包脚本
# 日期：2026-07-09

$projectDir = "C:\Users\ccc\Desktop\wukong-main"
$buildDir = "$projectDir\build"
$packageDir = "$projectDir\package_portable"
$releaseDir = "$buildDir\Release"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "悟空快递运费计算系统 - 快速打包" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查编译文件
Write-Host "检查编译文件..." -ForegroundColor Yellow
if (-not (Test-Path "$releaseDir\WukongFreight.exe")) {
    Write-Host "❌ 未找到编译文件！" -ForegroundColor Red
    Write-Host ""
    Write-Host "请先编译项目：" -ForegroundColor Yellow
    Write-Host "  cd $projectDir" -ForegroundColor Cyan
    Write-Host "  mkdir build && cd build" -ForegroundColor Cyan
    Write-Host "  cmake .." -ForegroundColor Cyan
    Write-Host "  cmake --build . --config Release" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "或者运行：" -ForegroundColor Yellow
    Write-Host "  .\build.ps1" -ForegroundColor Cyan
    exit 1
}
Write-Host "✅ 编译文件已找到" -ForegroundColor Green
Write-Host ""

# 2. 创建打包目录
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 2: 创建打包目录" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if (Test-Path $packageDir) {
    Remove-Item -Recurse -Force $packageDir
}
mkdir $packageDir | Out-Null
Write-Host "✅ 打包目录：$packageDir" -ForegroundColor Green
Write-Host ""

# 3. 复制主程序
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 3: 复制主程序" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Copy-Item "$releaseDir\WukongFreight.exe" "$packageDir\" -Force
Write-Host "✅ WukongFreight.exe" -ForegroundColor Green
Write-Host ""

# 4. 复制依赖库
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 4: 复制依赖库" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 查找 Qt 路径
$qtPath = $null
if (Test-Path "C:\Qt") {
    $qtPath = Get-ChildItem -Path "C:\Qt" -Directory | Where-Object { $_.Name -like "6.*" } | Select-Object -First 1
}

if ($qtPath) {
    $qtBinPath = Get-ChildItem -Path "$($qtPath.FullName)\msvc*\*\bin" -ErrorAction SilentlyContinue | Select-Object -First 1 | Split-Path
    
    Write-Host "Qt 路径：$qtBinPath" -ForegroundColor Yellow
    Write-Host ""
    
    # 复制 Qt DLL
    $qtDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Concurrent.dll", "Qt6Network.dll")
    foreach ($dll in $qtDlls) {
        $src = "$qtBinPath\$dll"
        if (Test-Path $src) {
            Copy-Item $src "$packageDir\" -Force
            Write-Host "✅ $dll" -ForegroundColor Green
        }
    }
    
    # 复制平台插件
    $platformsPath = "$qtBinPath\..\..\plugins\platforms\qwindows.dll"
    if (Test-Path $platformsPath) {
        mkdir "$packageDir\platforms" | Out-Null
        Copy-Item $platformsPath "$packageDir\platforms\" -Force
        Write-Host "✅ platforms\qwindows.dll" -ForegroundColor Green
    }
    
    # 复制样式插件
    $stylesPath = "$qtBinPath\..\..\plugins\styles\qwindowsvistastyle.dll"
    if (Test-Path $stylesPath) {
        mkdir "$packageDir\styles" | Out-Null
        Copy-Item $stylesPath "$packageDir\styles\" -Force
        Write-Host "✅ styles\qwindowsvistastyle.dll" -ForegroundColor Green
    }
} else {
    Write-Host "⚠️ 未找到 Qt，请手动复制依赖库" -ForegroundColor Yellow
}
Write-Host ""

# 5. 复制 VC++ 运行时
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 5: 复制 VC++ 运行时" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWherePath) {
    $vsPath = & $vsWherePath -latest -requires Microsoft.Component.MSBuild -property installationPath
    if ($vsPath) {
        $vcPath = Get-ChildItem -Path "$vsPath\VC\Redist\MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
        if ($vcPath) {
            $vcDlls = Get-ChildItem -Path "$($vcPath.FullName)\x64\Microsoft.VC*.CRT" -Filter "*.dll"
            foreach ($dll in $vcDlls) {
                Copy-Item $dll.FullName "$packageDir\" -Force
                Write-Host "✅ $($dll.Name)" -ForegroundColor Green
            }
        }
    }
}
Write-Host ""

# 6. 创建文档
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 6: 创建文档" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$readme = @"
# 悟空快递运费计算系统 v1.0.0

**开发**: 杭州喵喵至家网络有限公司  
**日期**: 2026-07-09

## 快速开始

1. 双击运行 WukongFreight.exe
2. 导入 Excel 账单
3. 点击"开始计算"
4. 导出结果

## 新功能

在主界面点击"规则说明"按钮，查看完整的计算规则。

## 系统要求

- Windows 10/11 (64 位)
- 4GB RAM 以上

## 技术支持

邮箱：support@example.com  
时间：周一至周五 9:00-18:00

**Copyright © 2026 杭州喵喵至家网络有限公司**
"@

$readme | Out-File -FilePath "$packageDir\README.md" -Encoding UTF8
Write-Host "✅ README.md" -ForegroundColor Green

$changelog = @"
# 更新日志

## v1.0.0 (2026-07-09)

### 新增
- 规则说明功能
- 完整的计算规则文档

### 修复
- 重量段边界条件错误
- 计算模式逻辑混乱
- Python/C++ 逻辑不一致
- 价格倒挂问题

详细文档请参考项目目录中的 COMPLETE_FIX_REPORT.md
"@

$changelog | Out-File -FilePath "$packageDir\CHANGELOG.md" -Encoding UTF8
Write-Host "✅ CHANGELOG.md" -ForegroundColor Green
Write-Host ""

# 7. 创建压缩包
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 7: 创建压缩包" -ForegroundColor Cyan
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
    Write-Host "安装包位置：" -ForegroundColor Yellow
    Write-Host "  $zipPath" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "文件大小：{0:N2} MB" -f $size -ForegroundColor Cyan
    Write-Host ""
    Write-Host "包含内容：" -ForegroundColor Yellow
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
    Write-Host "请检查是否有足够的磁盘空间或权限问题" -ForegroundColor Yellow
}
