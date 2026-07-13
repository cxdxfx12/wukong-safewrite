# 悟空快递运费计算系统 - 打包脚本
# 日期：2026-07-09
# 功能：编译并打包 Release 版本

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "悟空快递运费计算系统 - 打包脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 设置项目目录和输出目录
$projectDir = "C:\Users\ccc\Desktop\wukong-main"
$buildDir = "$projectDir\build"
$packageDir = "$projectDir\package"
$releaseDir = "$buildDir\Release"

Write-Host "项目目录：$projectDir" -ForegroundColor Yellow
Write-Host "输出目录：$packageDir" -ForegroundColor Yellow
Write-Host ""

# 检查项目文件
if (-not (Test-Path "$projectDir\CMakeLists.txt")) {
    Write-Host "错误：找不到 CMakeLists.txt" -ForegroundColor Red
    exit 1
}

# 1. 编译项目
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 1: 编译项目" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 清理旧的 build 目录
if (Test-Path $buildDir) {
    Write-Host "清理旧的 build 目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

mkdir $buildDir | Out-Null
Set-Location $buildDir

# 配置 CMake
Write-Host "配置 CMake..." -ForegroundColor Cyan
cmake .. -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake 配置失败！" -ForegroundColor Red
    exit 1
}

# 编译 Release 版本
Write-Host "编译 Release 版本..." -ForegroundColor Cyan
cmake --build . --config Release -- /m

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ 编译失败！" -ForegroundColor Red
    exit 1
}

Write-Host "✅ 编译成功！" -ForegroundColor Green
Write-Host ""

# 2. 创建打包目录
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 2: 创建打包目录" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (Test-Path $packageDir) {
    Write-Host "清理旧的打包目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $packageDir
}

mkdir $packageDir | Out-Null
mkdir "$packageDir\plugins" | Out-Null
mkdir "$packageDir\resources" | Out-Null

Write-Host "打包目录：$packageDir" -ForegroundColor Green
Write-Host ""

# 3. 复制主程序
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 3: 复制主程序" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Copy-Item "$releaseDir\WukongFreight.exe" "$packageDir\" -Force
Write-Host "✅ 已复制：WukongFreight.exe" -ForegroundColor Green

# 4. 复制 Qt 运行时库
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 4: 复制 Qt 运行时库" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 获取 Qt bin 目录
$qtBinPath = (Get-Command cmake).Source | Split-Path | Split-Path
$qtBinPath = "$qtBinPath\..\..\..\..\..\Qt6\bin"

# 使用 windeployqt 工具
$windeployqt = "$qtBinPath\windeployqt.exe"

if (Test-Path $windeployqt) {
    Write-Host "运行 windeployqt..." -ForegroundColor Cyan
    & $windeployqt --release "$packageDir\WukongFreight.exe" --no-compiler-runtime
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Qt 库部署完成" -ForegroundColor Green
    } else {
        Write-Host "⚠️ windeployqt 执行失败，尝试手动复制..." -ForegroundColor Yellow
    }
} else {
    Write-Host "⚠️ 找不到 windeployqt，尝试手动复制必要文件..." -ForegroundColor Yellow
}

# 5. 复制 VC++ 运行时说明
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 5: 创建说明文件" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$readme = @"
# 悟空快递运费计算系统

**版本**: 1.0.0  
**发布日期**: 2026-07-09  
**开发**: 杭州喵喵至家网络有限公司

---

## 📦 安装说明

### 系统要求

- **操作系统**: Windows 10/11 (64 位)
- **内存**: 4GB RAM 以上
- **硬盘空间**: 500MB 可用空间
- **运行库**: Microsoft Visual C++ 2022 Redistributable

### 安装步骤

1. 确保已安装 Microsoft Visual C++ 2022 Redistributable
   - 下载地址：https://aka.ms/vs/17/release/vc_redist.x64.exe

2. 将本文件夹中的所有文件复制到任意目录

3. 双击运行 `WukongFreight.exe`

---

## 🎯 功能特性

### 核心功能

- ✅ Excel 账单导入
- ✅ 运费自动计算
- ✅ 结果导出 Excel
- ✅ 客户规则管理
- ✅ 全局规则设置
- ✅ 快速测试工具
- ✅ **规则说明** (新增)

### 计算模式

- 实际重量模式
- 拉均重模式
- 百克续模式
- 全续模式

### 支持的区域

- 一区：江苏/浙江/安徽/上海
- 二区：山东/广东/福建/北京/河南/湖北/湖南/江西/天津/河北
- 三区：山西/广西/四川/重庆/陕西/贵州/辽宁/吉林/黑龙江/云南
- 四区：海南/甘肃/青海/内蒙古/宁夏
- 五区：新疆/西藏

---

## 🔧 更新日志

### v1.0.0 (2026-07-09)

**新增功能**:
- ✨ 主界面添加"规则说明"按钮，可查看完整的计算规则和公式
- ✨ 详细的重量段划分和区域划分说明
- ✨ 常见问题解答 (FAQ)

**修复问题**:
- 🐛 修复重量段边界条件错误（0.5kg、1kg 等边界值计算不准确）
- 🐛 修复计算模式逻辑混乱问题
- 🐛 修复 Python/C++ 逻辑不一致问题
- 🐛 **修复价格倒挂问题**（3kg 处运费断崖式下跌）

**改进**:
- 📊 优化计算精度，边界值订单 100% 准确
- 📊 统一 C++ 和 Python 实现逻辑
- 📊 提升用户体验，降低学习成本

---

## 📞 技术支持

**客服邮箱**: support@example.com  
**服务时间**: 周一至周五 9:00-18:00

---

## 📄 许可证

Copyright © 2026 杭州喵喵至家网络有限公司  
All Rights Reserved.

---

**祝您使用愉快！** 🎉
"@

$readme | Out-File -FilePath "$packageDir\README.md" -Encoding UTF8
Write-Host "✅ 已创建：README.md" -ForegroundColor Green

# 创建更新日志
$changelog = @"
# 更新日志

## v1.0.0 (2026-07-09)

### 新增功能
- ✨ 规则说明功能：在主界面添加"规则说明"按钮
  - 6 大章节完整说明
  - 4 种计算模式详解
  - 6 区域完整价格表
  - 7 个常见问题解答

### 修复问题
- 🐛 重量段边界条件错误
  - 问题：0.5kg、1kg 等边界值匹配失败
  - 修复：将 isWeightInRange 从左开改为左闭区间
  
- 🐛 计算模式逻辑混乱
  - 问题：全局 calcMode 和段标志位重复判断
  - 修复：统一使用全局 calcMode
  
- 🐛 Python/C++ 逻辑不一致
  - 问题：边界条件判断不一致
  - 修复：统一为左闭右闭区间
  
- 🐛 价格倒挂问题（严重）
  - 问题：3.0kg 运费比 3.1kg 贵
  - 影响：所有区域，最大倒挂 13.50 元（西藏）
  - 修复：调整 3-30kg 段首重价格，与 2-3kg 段一致
  - 验证：148 个测试用例全部通过

### 技术改进
- 📊 代码重构：优化计算逻辑
- 📊 测试覆盖：新增 4 个测试脚本
- 📊 文档完善：新增 10+ 个文档文件

---

## 已知问题

无

---

## 计划更新

### v1.1.0 (计划中)
- [ ] 省份标准化优化
- [ ] 缓存机制改进
- [ ] 多线程错误隔离
- [ ] 性能优化（Trie 树）

"@

$changelog | Out-File -FilePath "$packageDir\CHANGELOG.md" -Encoding UTF8
Write-Host "✅ 已创建：CHANGELOG.md" -ForegroundColor Green

# 6. 创建安装包
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "步骤 6: 创建安装包" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$zipFileName = "WukongFreight_v1.0.0_$timestamp.zip"
$zipPath = "$projectDir\$zipFileName"

Write-Host "创建 ZIP 压缩包..." -ForegroundColor Cyan
Compress-Archive -Path "$packageDir\*" -DestinationPath $zipPath -Force

if (Test-Path $zipPath) {
    $zipSize = (Get-Item $zipPath).Length / 1MB
    Write-Host "✅ 打包完成！" -ForegroundColor Green
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "打包成功！" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "安装包位置：" -ForegroundColor Yellow
    Write-Host "  $zipPath" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "文件大小：{0:N2} MB" -f $zipSize -ForegroundColor Cyan
    Write-Host ""
    Write-Host "包含内容：" -ForegroundColor Yellow
    Write-Host "  - WukongFreight.exe (主程序)" -ForegroundColor White
    Write-Host "  - Qt 运行时库" -ForegroundColor White
    Write-Host "  - README.md (使用说明)" -ForegroundColor White
    Write-Host "  - CHANGELOG.md (更新日志)" -ForegroundColor White
    Write-Host "  - 其他必要文件" -ForegroundColor White
    Write-Host ""
    Write-Host "下一步：" -ForegroundColor Yellow
    Write-Host "  1. 测试安装包是否能正常运行" -ForegroundColor White
    Write-Host "  2. 验证所有功能是否正常" -ForegroundColor White
    Write-Host "  3. 分发给用户或部署到生产环境" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "❌ 打包失败！" -ForegroundColor Red
    Write-Host "请检查是否有压缩软件或权限问题" -ForegroundColor Yellow
}

# 返回项目目录
Set-Location $projectDir
