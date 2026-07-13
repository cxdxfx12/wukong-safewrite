# 悟空快递运费计算系统 - 打包指南

**日期**: 2026-07-09  
**版本**: v1.0.0

---

## 📦 打包方式

提供两种打包方式：

### 方式 1: 完整安装包（推荐）

**脚本**: `package.ps1`  
**特点**: 
- 自动编译项目
- 使用 windeployqt 部署 Qt 依赖
- 创建标准安装包
- 包含完整文档

**使用方法**:
```powershell
cd C:\Users\ccc\Desktop\wukong-main
.\package.ps1
```

**输出**:
- `WukongFreight_v1.0.0_yyyyMMdd_HHmmss.zip`
- 大小：约 80-120 MB
- 包含：主程序 + Qt 运行时 + 文档

---

### 方式 2: 便携版

**脚本**: `package_portable.ps1`  
**特点**:
- 手动复制依赖库
- 不依赖 windeployqt
- 更小的包体积
- 适合快速分发

**使用方法**:
```powershell
cd C:\Users\ccc\Desktop\wukong-main
.\package_portable.ps1
```

**输出**:
- `WukongFreight_v1.0.0_Portable_yyyyMMdd_HHmmss.zip`
- 大小：约 50-80 MB
- 包含：主程序 + 必要依赖 + 文档

---

## 🚀 快速开始

### 最简单的方式

```powershell
# 进入项目目录
cd C:\Users\ccc\Desktop\wukong-main

# 运行便携版打包脚本
.\package_portable.ps1

# 等待编译和打包完成（约 2-5 分钟）
```

打包完成后会在项目目录生成 ZIP 文件。

---

## 📋 打包流程

### 完整流程

```
1. 编译项目
   ↓
2. 创建打包目录
   ↓
3. 复制主程序
   ↓
4. 复制依赖库（Qt、VC++）
   ↓
5. 创建文档（README、CHANGELOG）
   ↓
6. 压缩成 ZIP
   ↓
7. 完成！
```

---

## 📁 打包内容

### 标准包内容

```
WukongFreight_v1.0.0_yyyyMMdd_HHmmss/
├── WukongFreight.exe          # 主程序
├── Qt6Core.dll                # Qt 核心库
├── Qt6Gui.dll                 # Qt GUI 库
├── Qt6Widgets.dll            # Qt 控件库
├── Qt6Concurrent.dll         # Qt 并发库
├── Qt6Network.dll            # Qt 网络库
├── *.dll                     # VC++ 运行时
├── platforms/
│   └── qwindows.dll          # Windows 平台插件
├── styles/
│   └── qwindowsvistastyle.dll # Windows 样式插件
├── README.md                 # 使用说明
└── CHANGELOG.md              # 更新日志
```

---

## 🧪 测试安装包

### 测试步骤

1. **解压到临时目录**
   ```powershell
   mkdir C:\temp\test
   Expand-Archive -Path WukongFreight_v1.0.0_*.zip -DestinationPath C:\temp\test
   ```

2. **运行程序**
   ```powershell
   cd C:\temp\test
   .\WukongFreight.exe
   ```

3. **验证功能**
   - [ ] 主界面正常显示
   - [ ] "规则说明"按钮存在
   - [ ] 点击"规则说明"能打开对话框
   - [ ] 规则内容正确显示
   - [ ] 导入 Excel 功能正常
   - [ ] 运费计算正常
   - [ ] 导出结果正常

4. **清理测试**
   ```powershell
   Remove-Item -Recurse -Force C:\temp\test
   ```

---

## 🛠️ 故障排除

### 问题 1: 编译失败

**错误**: `CMake configuration failed`

**解决方法**:
```powershell
# 检查 Visual Studio 安装
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest

# 确认已安装 C++ 桌面开发组件
```

### 问题 2: 找不到 Qt

**错误**: `Qt6 not found`

**解决方法**:
```powershell
# 检查 Qt 安装路径
Get-ChildItem C:\Qt -Directory

# 设置 Qt6Dir 环境变量
$env:Qt6Dir = "C:\Qt\6.x.x\msvc*\*\bin"
```

### 问题 3: 打包失败

**错误**: `Compress-Archive failed`

**解决方法**:
```powershell
# 检查是否有足够的磁盘空间
Get-PSDrive C:

# 手动创建 ZIP
$shell = New-Object -ComObject Shell.Application
$zip = $shell.Namespace(0).CopyHere
```

### 问题 4: 程序无法启动

**错误**: `找不到 Qt6Core.dll`

**解决方法**:
- 确保所有 DLL 文件都在同一目录
- 检查 `platforms\qwindows.dll` 是否存在
- 确认 VC++ 运行时已安装

---

## 📊 包大小优化

### 减小包体积的方法

1. **使用 Release 模式编译**
   ```powershell
   cmake --build . --config Release
   ```

2. **移除不必要的 Qt 模块**
   - 只复制实际使用的 DLL
   - 移除 Qt6Qml、Qt6Quick 等不需要的模块

3. **压缩级别**
   ```powershell
   Compress-Archive -CompressionLevel Optimal
   ```

4. **分离调试符号**
   - 不包含 .pdb 文件
   - 单独提供调试包

---

## 📝 分发建议

### 内部测试

- 使用便携版快速打包
- 分发给测试人员
- 收集反馈

### 正式发布

- 使用完整安装包
- 包含完整文档
- 数字签名（可选）

### 客户交付

- 提供 README.md
- 提供 CHANGELOG.md
- 提供技术支持联系方式

---

## 🔗 相关文档

- [编译说明](file:///C:/Users/ccc/Desktop/wukong-main/BUILD_INSTRUCTIONS.md)
- [功能实现报告](file:///C:/Users/ccc/Desktop/wukong-main/FINAL_IMPLEMENTATION_REPORT.md)
- [规则说明功能](file:///C:/Users/ccc/Desktop/wukong-main/RULE_HELP_SUMMARY.md)
- [完整修复报告](file:///C:/Users/ccc/Desktop/wukong-main/COMPLETE_FIX_REPORT.md)

---

## ✅ 打包检查清单

- [ ] 项目编译成功
- [ ] 所有依赖已复制
- [ ] 文档已创建
- [ ] ZIP 压缩包已生成
- [ ] 测试安装包能正常运行
- [ ] 所有功能测试通过
- [ ] 包大小符合预期

---

**打包完成时间**: 2026-07-09  
**状态**: ✅ 准备就绪  
**下一步**: 运行 `.\package_portable.ps1` 开始打包
