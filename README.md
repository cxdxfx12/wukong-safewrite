# 悟空运费结算

快递网点财务运费结算系统，支持多快递公司（三通一达+顺丰+京东）报价表管理、Excel 批量导入、自动运费计算、图表统计分析。

**技术栈：** Qt 6.12 + C++17 + CMake + Ninja + QXlsx

---

## 目录结构

```
wukong-main/
├── CMakeLists.txt              # CMake 构建配置
├── build.bat / build.ps1       # 一键构建脚本
├── build/                      # 构建输出目录（exe + dll）
│
├── src/
│   ├── main.cpp                # 入口：QApplication → LoginDialog → MainWindow
│   ├── MainWindow.h/cpp        # 主界面：工具栏 + 数据预览 + 控制面板
│   ├── MainWindow.ui           # 主界面 UI 布局文件
│   │
│   ├── DataModel/              # 数据层
│   │   ├── OrderData.h/cpp     #   订单数据结构
│   │   ├── PriceTable.h/cpp    #   报价表：WeightSegment / PriceRule / CustomerRule / GlobalRules
│   │   ├── CalculationRule.h/cpp # 计算规则：4种模式(实际重量/拉均重/百克续/全续)
│   │   ├── CustomerStats.h     #   客户/店铺统计数据 + computeCustomerStats / computeStoreStats
│   │   ├── MappingTemplate.h/cpp # ★ 表头映射模板 + MappingTemplateManager（持久化）
│   │   ├── CalculationHistory.h  # 计算历史记录
│   │   └── Customer.h          #   客户基础类型
│   │
│   ├── Calculation/            # 计算引擎
│   │   ├── FreightCalculator.h/cpp  # 运费计算器（信号驱动）
│   │   ├── SimdCalculator.h/cpp     # SIMD 批量计算优化
│   │   ├── RuleManager.h/cpp        # ★ 规则管理器（客户规则CRUD + 报价表 + 持久化）
│   │   └── ThreadPool.h/cpp         # 线程池
│   │
│   ├── Excel/                  # Excel 读写
│   │   ├── ExcelEngine.h/cpp   #   QXlsx 核心封装（流式 SAX 读写）
│   │   ├── ExcelImporter.h/cpp #   ★ Excel 导入 + autoDetectColumns 列映射
│   │   └── ExcelExporter.h/cpp #   Excel 导出
│   │
│   ├── UI/                     # 界面组件
│   │   ├── CustomerDialog.h/cpp      # 客户管理对话框
│   │   ├── PriceTableDialog.h/cpp    # 报价表编辑
│   │   ├── RuleManagerDialog.h/cpp   # 规则管理（基于 .ui 文件）
│   │   ├── GlobalRulesDialog.h/cpp   # 全局规则（活动加价/临时加价/省份加价）
│   │   ├── HeaderMappingDialog.h/cpp # ★ 表头映射对话框（模板选择+保存+预览）
│   │   ├── QuickTestWidget.h/cpp     # 快速测试
│   │   ├── RuleHelpDialog.h/cpp      # 规则说明
│   │   ├── HistoryDialog.h/cpp       # 历史记录
│   │   ├── ChartDialog.h/cpp         # ★ 统计图表对话框（4个Tab）
│   │   ├── ChartWidgets.h/cpp        #   图表组件：StatCard / DonutChart / BarChart
│   │   ├── BannerWidget.h/cpp        #   滚动横幅
│   │   └── ProgressWidget.h/cpp      #   进度组件
│   │
│   ├── Auth/                   # 授权
│   │   ├── LoginDialog.h/cpp   #   登录对话框
│   │   └── LicenseManager.h/cpp #  许可证管理
│   │
│   └── Utils/                  # 工具
│       ├── Logger.h/cpp        #   日志
│       ├── ConfigManager.h/cpp #   配置管理
│       └── ProvinceUtils.h/cpp #   省份工具
│
├── QXlsx/                      # QXlsx 子模块（Excel 读写库）
├── resources/                  # 资源文件
│   ├── style.qss               #   全局 QSS 样式表（浅色主题、橙色强调色）
│   ├── app_icon.ico/png        #   应用图标
│   └── resources.qrc           #   Qt 资源索引
│
├── deploy/                     # 部署相关
├── package/                    # 打包输出
└── tools/                      # 辅助工具脚本
```

---

## 构建方法

### 环境要求
- **Qt 6.12+** (`E:/Qt/6.12.0/mingw_64`)
- **CMake 3.16+** + **Ninja**
- **MinGW 13.1** (`E:/Qt/Tools/mingw1310_64`)
- QXlsx 已作为子模块包含在项目中

### 构建命令
```bash
cd build
cmake .. -G "Ninja" \
  -DCMAKE_PREFIX_PATH="E:/Qt/6.12.0/mingw_64" \
  -DCMAKE_BUILD_TYPE=Release
ninja
```

或直接运行 `build.bat`。

### 打包
```bash
windeployqt --release 悟空结算.exe
# 然后将 exe + dll + platforms/styles/imageformats/ 打包为 zip
```

---

## 核心数据流

```
┌──────────┐    ┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│ Excel 文件 │───→│ ExcelImporter │───→│ OrderData 列表 │───→│ QTableView  │
│ (.xlsx)   │    │ autoDetect   │    │ (内存)        │    │ (数据预览)   │
└──────────┘    └─────────────┘    └──────┬───────┘    └─────────────┘
                                          │
                          ┌───────────────┘
                          ▼
                   ┌──────────────┐
                   │ RuleManager   │ ← 报价表 + 客户规则 + 全局规则
                   └──────┬───────┘
                          ▼
                   ┌──────────────┐
                   │ FreightCalc   │ ← 4种计算模式
                   │ (SIMD加速)    │
                   └──────┬───────┘
                          ▼
                   ┌──────────────┐
                   │ OrderData     │ ← freight / usedRule / errorMessage 已填充
                   │ (结果回写)     │
                   └──────┬───────┘
                          ▼
                   ┌──────────────┐
                   │ ExcelExporter │ → 导出 xlsx
                   │ ChartDialog   │ → 统计图表
                   └──────────────┘
```

---

## 关键设计决策

### 1. 列映射机制

Excel 列 → 内部字段的映射有**三层策略**：

| 策略 | 类/文件 | 说明 |
|---|---|---|
| 自动检测 | `ExcelImporter::autoDetectColumns()` | 关键词匹配，如 `"结算重量" → 实际重量`，优先级=精确匹配(100) > 长关键词 > 短关键词 |
| 映射模板 | `MappingTemplate` + `MappingTemplateManager` | 预置6套快递公司模板，精确别名优先、包含匹配兜底 |
| 手动调整 | `HeaderMappingDialog` | 导入后用户可调整关键字段映射，触发 re-import |

关键词定义在 [ExcelImporter.cpp:43-54](src/Excel/ExcelImporter.cpp#L43-L54)。

### 2. 计算规则系统

4 种计算模式：[CalculationRule.h](src/DataModel/CalculationRule.h)

| 模式 | 逻辑 |
|---|---|
| 实际重量 | `max(实重, 体积重)` 匹配重量段 → `首重价 + 续重价 × 续重重量` |
| 拉均重 | 先算客户平均重量，超基准加价，再按实际重量计算 |
| 百克续 | 按每 100g 阶梯计费 |
| 全续 | 全部按续重价计算 |

### 3. 报价表体系

[PriceTable.h](src/DataModel/PriceTable.h) 定义了完整的价格规则链：

```
WeightSegment → PriceRule → CustomerRule → GlobalRules
   (重量段)      (区域/省份)    (客户定制)     (全局叠加)
                                          ├─ ActivityRule（活动加价）
                                          ├─ TempPriceIncrease（临时加价）
                                          └─ ProvincePriceIncrease（省份加价）
```

每套快递公司（申通/中通/圆通/韵达/顺丰/京东）有独立报价表，按5个区域（一区~五区）+ 省份 + 6个重量段组织。

### 4. 两种统计维度

| 维度 | 分组字段 | 函数 | 用途 |
|---|---|---|---|
| 结算客户 | `OrderData.client` | `computeCustomerStats()` | 财务对账（ChartDialog 前3个Tab） |
| 店铺 | `OrderData.customer` | `computeStoreStats()` | 运营分析（ChartDialog 第4个Tab） |

### 5. 持久化文件

| 文件 | 路径 | 内容 |
|---|---|---|
| `rules.json` | `AppData/杭州喵喵至家网络有限公司/悟空运费结算/` | 客户规则 + 报价表 + 全局规则 |
| `column_mappings.json` | 同上 | 映射模板（预置6套+自定义） |
| `calculation_history.json` | 同上 | 计算历史记录 |
| `freight_calculator.log` | 同上 | 运行日志 |

---

## 扩展指南

### 新增快递公司

1. **报价表数据** — 在 [RuleManager.cpp](src/Calculation/RuleManager.cpp) 的 `initAllPriceTables()` 中添加新的 `CourierPrices` 条目
2. **UI 下拉** — 在 [MainWindow.cpp](src/MainWindow.cpp) 和 [CustomerDialog.cpp](src/UI/CustomerDialog.cpp) 的快递公司下拉中添加选项
3. **映射模板** — 在 [MappingTemplate.cpp](src/DataModel/MappingTemplate.cpp) 的 `builtinTemplates()` 中添加默认模板

### 新增计算模式

1. 在 [CalculationRule.h](src/DataModel/CalculationRule.h) 的 `Mode` 枚举中添加新模式
2. 实现计算公式 `calculateXxx()`
3. 在 [MainWindow.ui](src/MainWindow.ui) 的 `modeCombo` 下拉中添加选项
4. 如需性能优化，在 [SimdCalculator.cpp](src/Calculation/SimdCalculator.cpp) 中添加 SIMD 版本

### 新增图表类型

1. 在 [ChartWidgets.h/cpp](src/UI/ChartWidgets.h) 中创建新的 QWidget 子类
2. 实现 `paintEvent()` 做自绘
3. 在 [ChartDialog.cpp](src/UI/ChartDialog.cpp) 的对应 Tab 方法中实例化

### 修改 Excel 列映射规则

- **关键词匹配：** 编辑 [ExcelImporter.cpp](src/Excel/ExcelImporter.cpp) 中 `autoDetectColumns()` 的 `keywordMap`
- **映射模板：** 编辑 [MappingTemplate.cpp](src/DataModel/MappingTemplate.cpp) 的 `builtinTemplates()`
- 映射模板会在**首次启动时自动创建**（`ensureBuiltinsExist()`），后续修改通过 UI 保存

### 修改 UI 样式

- **全局样式：** 编辑 [resources/style.qss](resources/style.qss)（浅色主题，橙色 `#ff7f20` 为主色调）
- **按钮选择器：** `#primaryButton`、`#secondaryButton`、`#smallButton`、`#dangerButton` 已预定义
- **新增对话框：** 使用这些预定义选择器即可自动匹配风格，**不需要手写 QSS**

---

## 重要注意事项

1. **文件编码：** 项目源文件使用 **GBK/GB2312** 编码（中文 Windows 默认），非 UTF-8。编辑时注意保持原有编码。
2. **RC 编译：** `CMAKE_RC_COMPILER` 必须指向 Qt 工具链的 64 位 windres，不能用系统 `C:\Windows\MinGW` 的 32 位版本。
3. **`autoDetectColumns` 位置：** 此方法必须在 `signals:` 之前声明（在 `public:` 区域），否则 MOC 会将其误作 signal 并生成重复定义。
4. **`importFilesAsync` 参数：** `useExistingMapping=false` 自动检测列映射，`=true` 使用已有的 `m_lastColumnMapping`（表头映射对话框调整后重新导入时使用）。
5. **QSS 全局生效：** 项目在 `main.cpp` 中通过 `:/style.qss` 设置全局样式表，新增对话框不需要单独设置样式（除非有特殊需求）。
