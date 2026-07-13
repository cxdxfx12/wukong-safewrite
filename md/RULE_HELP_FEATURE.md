# 规则说明功能实现文档

**日期**: 2026-07-09  
**功能**: 在主界面添加"规则说明"按钮，解释规则设置和计算逻辑

---

## 📋 实现内容

### 1. 新增文件

| 文件 | 类型 | 说明 |
|------|------|------|
| `src/UI/RuleHelpDialog.h` | 头文件 | 规则说明对话框类定义 |
| `src/UI/RuleHelpDialog.cpp` | 实现文件 | 对话框实现，包含完整的规则说明内容 |

### 2. 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/MainWindow.h` | 添加 `onRuleHelpClicked()` 槽函数声明 |
| `src/MainWindow.cpp` | 添加 `RuleHelpDialog` 头文件引用、按钮连接、实现函数 |
| `src/MainWindow.ui` | 添加 `ruleHelpBtn` 按钮 |

---

## 🎨 功能特性

### 1. 规则说明内容

对话框包含以下章节：

1. **计算模式说明**
   - 实际重量模式
   - 拉均重模式
   - 百克续模式
   - 全续模式

2. **重量段划分**
   - 一至五区的详细价格表
   - 首重价、续重价清晰标注

3. **区域划分**
   - 每个区域包含的省份列表

4. **计算公式**
   - 标准计算公式
   - 实际计算示例（一区 4.5kg、二区 10kg）

5. **特殊规则**
   - 活动加价
   - 临时加价
   - 省份加价
   - 无重量默认价
   - 客户自定义价格

6. **常见问题**
   - 7 个常见问题及解答
   - 涵盖边界值、体积重、计算精度等

### 2. UI 设计

- **窗口大小**: 800×600（最小）
- **内容展示**: 使用 `QTextBrowser` 支持 Markdown 渲染
- **链接支持**: 支持外部链接打开
- **目录导航**: 内部锚点链接，方便跳转

---

## 🔧 技术实现

### 对话框类结构

```cpp
class RuleHelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit RuleHelpDialog(QWidget *parent = nullptr);
    ~RuleHelpDialog();

private:
    void setupContent();
    QString generateHelpContent();  // 生成 Markdown 内容
};
```

### Markdown 内容生成

使用 `QString` 原始字符串（R"(...)"）存储完整的 Markdown 文档，通过 `QTextBrowser::setMarkdown()` 渲染。

### 按钮布局

在主界面工具栏中添加按钮，位置在"全局规则"右侧：

```
[导入 Excel] [开始计算] [导出结果] | [快速测试] [规则管理] [全局规则] [规则说明]
```

---

## 📝 使用说明

### 用户操作

1. 启动软件后，在主界面顶部工具栏找到"**规则说明**"按钮
2. 点击按钮，弹出规则说明对话框
3. 可滚动查看完整内容
4. 点击目录中的链接可跳转到对应章节
5. 关闭窗口返回主界面

### 内容更新

如需修改规则说明内容，编辑 `src/UI/RuleHelpDialog.cpp` 中的 `generateHelpContent()` 函数：

```cpp
QString RuleHelpDialog::generateHelpContent()
{
    return R"(
# 运费计算规则说明

// 在此处修改 Markdown 内容
)";
}
```

---

## 🧪 测试建议

### 功能测试

1. **按钮显示**: 确认"规则说明"按钮在主界面正确显示
2. **对话框打开**: 点击按钮能正常打开对话框
3. **内容渲染**: Markdown 格式正确渲染（标题、表格、列表、代码块）
4. **链接跳转**: 目录链接能正确跳转到对应章节
5. **窗口大小**: 对话框大小合适，内容可滚动

### 兼容性测试

1. **不同分辨率**: 在 1366×768、1920×1080 等分辨率下测试
2. **DPI 缩放**: 在 100%、125%、150% 缩放比例下测试
3. **操作系统**: Windows 10/11 测试

---

## 📦 编译部署

### 编译步骤

```bash
cd C:\Users\ccc\Desktop\wukong-main

# 清理并重新编译
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 依赖检查

确保项目包含以下 Qt 模块：

```cmake
find_package(Qt5 COMPONENTS Widgets REQUIRED)
```

`QTextBrowser` 和 `QMarkdown` 支持需要 Qt 5.14+ 版本。

---

## 🎯 后续优化建议

### 短期优化

1. **内容本地化**: 支持中英文切换
2. **搜索功能**: 添加内容搜索框
3. **打印支持**: 允许用户打印规则说明

### 长期优化

1. **在线帮助**: 从服务器加载最新规则说明
2. **视频教程**: 嵌入视频链接
3. **智能搜索**: 根据用户问题智能推荐规则章节

---

## 🔗 相关文件

- [RuleHelpDialog.h](file:///C:/Users/ccc/Desktop/wukong-main/src/UI/RuleHelpDialog.h)
- [RuleHelpDialog.cpp](file:///C:/Users/ccc/Desktop/wukong-main/src/UI/RuleHelpDialog.cpp)
- [MainWindow.h](file:///C:/Users/ccc/Desktop/wukong-main/src/MainWindow.h)
- [MainWindow.cpp](file:///C:/Users/ccc/Desktop/wukong-main/src/MainWindow.cpp)
- [MainWindow.ui](file:///C:/Users/ccc/Desktop/wukong-main/src/MainWindow.ui)

---

**实现完成时间**: 2026-07-09  
**开发者**: AI Code Assistant  
**状态**: ✅ 已完成，待编译测试
