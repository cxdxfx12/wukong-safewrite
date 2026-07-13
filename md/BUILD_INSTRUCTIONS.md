# 编译说明

**日期**: 2026-07-09  
**项目**: 悟空快递运费计算系统 (wukong-main)

---

## 📦 新增文件

本次更新添加了规则说明功能，新增以下文件：

1. `src/UI/RuleHelpDialog.h` - 规则说明对话框头文件
2. `src/UI/RuleHelpDialog.cpp` - 规则说明对话框实现文件
3. `src/UI/RuleHelpDialog.ui` - 规则说明对话框 UI 文件（可选，当前使用代码创建 UI）

---

## 🔧 修改文件

1. `CMakeLists.txt` - 添加 RuleHelpDialog 到源文件列表
2. `src/MainWindow.h` - 添加 `onRuleHelpClicked()` 槽函数
3. `src/MainWindow.cpp` - 添加实现和按钮连接
4. `src/MainWindow.ui` - 添加"规则说明"按钮

---

## 🚀 编译步骤

### 方法 1: 使用命令行

```bash
# 进入项目目录
cd C:\Users\ccc\Desktop\wukong-main

# 创建或清理 build 目录
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
}
mkdir build
cd build

# 配置 CMake
cmake ..

# 编译 Release 版本
cmake --build . --config Release

# 或编译 Debug 版本
cmake --build . --config Debug
```

### 方法 2: 使用 Qt Creator

1. 打开 Qt Creator
2. 文件 → 打开文件 → 选择 `CMakeLists.txt`
3. 等待 CMake 配置完成
4. 点击"构建"按钮（锤子图标）
5. 或按 `Ctrl+B`

### 方法 3: 使用 Visual Studio

1. 文件 → 打开 → 文件夹 → 选择 `wukong-main` 目录
2. Visual Studio 会自动配置 CMake
3. 生成 → 全部生成
4. 或按 `Ctrl+Shift+B`

---

## ✅ 验证编译成功

编译成功后，应该能看到：

```
Build succeeded
    0 Warning(s)
    0 Error(s)

Time Elapsed 00:01:23.45
```

生成的可执行文件位置：
```
C:\Users\ccc\Desktop\wukong-main\build\Release\WukongFreight.exe
```

---

## 🐛 可能的编译错误及解决方法

### 错误 1: 找不到 RuleHelpDialog.h

```
error: no such file or directory: 'src/UI/RuleHelpDialog.h'
```

**解决方法**: 确认文件已创建，路径正确。

### 错误 2: 未定义的引用 `onRuleHelpClicked`

```
error LNK2019: unresolved external symbol
```

**解决方法**: 确保 `MainWindow.cpp` 中实现了 `onRuleHelpClicked()` 函数。

### 错误 3: QTextBrowser 缺少 setMarkdown 方法

```
error: 'class QTextBrowser' has no member named 'setMarkdown'
```

**解决方法**: 确认使用 Qt 5.14+ 或 Qt6。本项目使用 Qt6，应该没问题。

### 错误 4: CMake 找不到新增文件

```
CMake Error: Cannot find source file
```

**解决方法**: 确认 `CMakeLists.txt` 中已添加 RuleHelpDialog 相关文件。

---

## 🧪 运行测试

编译成功后，运行程序测试：

```bash
# 在 build/Release 目录运行
.\build\Release\WukongFreight.exe
```

### 测试清单

1. **主界面显示**
   - [ ] 程序正常启动
   - [ ] 主界面显示正常
   - [ ] 工具栏显示"规则说明"按钮

2. **规则说明功能**
   - [ ] 点击"规则说明"按钮
   - [ ] 弹出规则说明对话框
   - [ ] 对话框显示完整内容
   - [ ] Markdown 格式正确渲染
   - [ ] 表格显示正常
   - [ ] 目录链接可点击
   - [ ] 关闭窗口正常

3. **原有功能**
   - [ ] 导入 Excel 正常
   - [ ] 运费计算正常
   - [ ] 导出结果正常
   - [ ] 规则管理正常
   - [ ] 全局规则正常

---

## 📝 注意事项

1. **Qt 版本**: 项目使用 Qt6，确保已安装 Qt 6.x
2. **编译器**: 建议使用 MSVC 2019 或更新版本
3. **依赖**: 确保 QXlsx 子模块已正确初始化

---

## 🔗 相关文档

- [功能实现文档](file:///C:/Users/ccc/Desktop/wukong-main/RULE_HELP_FEATURE.md)
- [UI 布局示意图](file:///C:/Users/ccc/Desktop/wukong-main/UI_LAYOUT_DIAGRAM.md)
- [实现总结](file:///C:/Users/ccc/Desktop/wukong-main/RULE_HELP_SUMMARY.md)
- [完整修复报告](file:///C:/Users/ccc/Desktop/wukong-main/COMPLETE_FIX_REPORT.md)

---

**文档生成**: 2026-07-09  
**状态**: 待编译验证
