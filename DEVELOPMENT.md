# 悟空运费结算 — 二次开发指南

> 版本 v1.2.2 | Qt 6.12 + C++17 + CMake + QXlsx  
> 用途：快递网点财务运费结算  
> 规模：支持百万行级 Excel 批量导入、6 家快递公司报价表、4 种计算模式

---

## 一、环境搭建

### 必需软件

| 工具 | 路径 |
|---|---|
| Qt 6.12 | `E:/Qt/6.12.0/mingw_64` |
| CMake 3.16+ | `E:/Qt/Tools/CMake_64` |
| Ninja | `E:/Qt/Tools/Ninja` |
| MinGW 13.1 | `E:/Qt/Tools/mingw1310_64` |
| Inno Setup 6 | `E:/Program Files (x86)/Inno Setup 6` (安装包用) |

### 构建命令

```bash
cd build
cmake .. -G "Ninja" \
  -DCMAKE_MAKE_PROGRAM="E:/Qt/Tools/Ninja/ninja.exe" \
  -DCMAKE_PREFIX_PATH="E:/Qt/6.12.0/mingw_64" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"
ninja
```

### 打包

```bash
# ZIP 包
cd build
windeployqt --release 悟空结算.exe
# 然后将 exe + dll + platforms/styles/imageformats/ 打包为 zip

# 安装包
cd ../installer
"E:/Program Files (x86)/Inno Setup 6/ISCC.exe" setup.iss
```

---

## 二、项目架构

### 目录总览

```
src/
├── main.cpp                    # 入口：QApplication → LoginDialog → MainWindow
├── MainWindow.h/cpp            # 主界面：工具栏 + 数据预览 + 控制面板
├── MainWindow.ui               # Qt Designer UI 文件
│
├── DataModel/                  # 数据层——所有数据结构
│   ├── OrderData.h             #   订单数据（运单号/重量/运费/省份...）
│   ├── PriceTable.h            # ★ 报价表体系（核心）
│   │   ├── WeightSegment       #     重量段（min/max/首重/续重）
│   │   ├── PriceRule           #     价格规则（区域+省份+重量段列表）
│   │   ├── CustomerRule        #     客户规则（客户名+映射表头+报价+快递）
│   │   ├── GlobalRules         #     全局规则（活动/临时/省份加价 + 拉均重）
│   │   ├── PriceIncreaseRule   #     统一加价规则（3种模式）
│   │   └── IncreaseMode        #     枚举：PerTicketFixed/Percent/PerKg
│   ├── CalculationRule.h       #   计算策略（4种重量模式+省份匹配+加价分派）
│   ├── CustomerStats.h         #   客户/店铺统计 + computeCustomerStats/computeStoreStats
│   ├── MappingTemplate.h       #   表头映射模板 + MappingTemplateManager（JSON持久化）
│   └── CalculationHistory.h    #   计算历史记录
│
├── Calculation/                # 计算引擎
│   ├── FreightCalculator.h/cpp #   运费计算器（核心算法）
│   │   ├── calculateFreightDetail()  # 单条计算：匹配→计费→加价
│   │   ├── findPriceRule()           # 省份查价（3重匹配）
│   │   └── calculateBatch()         # 批量计算
│   ├── SimdCalculator.h/cpp    #   SIMD 加速批量计算（AVX2/SSE4.1）
│   ├── RuleManager.h/cpp       #   规则管理器（CRUD + JSON 持久化 + 6套报价表）
│   └── ThreadPool.h/cpp        #   线程池
│
├── Excel/                      # Excel 读写
│   ├── ExcelEngine.h/cpp       #   QXlsx SAX 流式读写核心
│   │   ├── readExcel()         #    读取全部 Sheet
│   │   ├── getHeaders()        #    读表头
│   │   ├── countRowsFast()     #    快速计数行数
│   │   └── writeExcel()/writeExcelFast()  # 写出
│   ├── ExcelImporter.h/cpp     #   Excel 导入 + autoDetectColumns 列映射
│   └── ExcelExporter.h/cpp     #   Excel 导出 + 汇总表
│
├── UI/                         # 界面组件
│   ├── HeaderMappingDialog     # ★ 表头映射（模板选择/保存/预览）
│   ├── GlobalRulesDialog       #   全局规则（活动/临时/省份加价）
│   ├── CustomerDialog          #   客户管理
│   ├── RuleManagerDialog       #   规则管理
│   ├── ChartDialog             #   统计图表（4个Tab）
│   ├── ChartWidgets            #   图表组件（StatCard/DonutChart/BarChart）
│   ├── QuickTestWidget         #   快速测试运费
│   └── HistoryDialog           #   历史记录
│
├── Auth/                       # 授权
│   ├── LoginDialog             #   登录对话框
│   └── LicenseManager          #   许可证管理
│
└── Utils/                      # 工具
    ├── Logger                  #   日志
    ├── ProvinceUtils           #   省份标准化
    └── ConfigManager           #   配置
```

### 核心数据流

```
Excel 文件 → ExcelImporter(列映射)
  → ExcelEngine::readExcel(SAX解析)
  → QList<OrderData> (原始数据)
  → 运单号去重 (O(n) QSet过滤)
  → 表头映射 → 用户确认列
  → FreightCalculator::calculateFreightDetail()
      ├─ findPriceRule()     → 匹配省份报价
      ├─ getCalcFunc()       → 分派重量计算
      ├─ applyPriceIncreases → 活动加价（日期过滤）
      ├─ applyPriceIncreases → 临时加价（日期过滤）
      └─ getIncreaseFunc     → 省份加价（省份匹配）
  → 结果写回 OrderData.freight
  → ExcelExporter 导出 / ChartDialog 统计
```

---

## 三、关键设计说明

### 1. 列映射三层策略

| 层 | 实现 | 说明 |
|---|---|---|
| 自动检测 | `ExcelImporter::autoDetectColumns()` | 关键词优先级打分（精确=100分 > 长关键词 > 短关键词） |
| 模板匹配 | `MappingTemplate::applyTo()` | 6套快递公司预设模板，精确别名→包含匹配 |
| 手动调整 | `HeaderMappingDialog` | 导入前弹列选择对话框，6关键字段锁定 |

### 2. 加价模式函数指针表

```cpp
// PriceTable.h — 消除所有 switch/case
using IncreaseFunc = double (*)(double freight, double weight, double amount);
inline double incFixedPerTicket(...) { return freight + amount; }
inline double incPercentPerTicket(...) { return freight * (1.0 + amount); }
inline double incPerKg(...) { return freight + weight * amount; }
inline IncreaseFunc getIncreaseFunc(IncreaseMode mode) {
    static const IncreaseFunc table[] = { incFixedPerTicket, incPercentPerTicket, incPerKg };
    return table[static_cast<int>(mode)];
}
```

### 3. 计算模式函数指针表

```cpp
// CalculationRule.cpp
CalcSegmentFunc getCalcFunc(Mode mode) {
    switch (mode) {
    case FullAdditional: return calculateFullAdditional;
    case HundredGram:    return calculateHundredGram;
    default:             return calculateStandard;
    }
}
```

### 4. 报价表结构

```
WeightSegment (重量段)
  └── PriceRule (区域+省份+段列表)
       └── CustomerRule (客户定制)
            └── GlobalRules (全局叠加)
                 ├── activityRules        (活动加价)
                 ├── tempPriceIncreases   (临时加价)
                 └── provincePriceIncreases (省份加价)
```

### 5. 持久化文件

| 文件 | 路径 | 内容 |
|---|---|---|
| `rules.json` | AppData/杭州喵喵至家网络有限公司/悟空运费结算/ | 客户规则 + 报价表 + 全局规则 |
| `column_mappings.json` | 同上 | 映射模板（6套默认+自定义） |
| `calculation_history.json` | 同上 | 计算历史 |

### 6. 省份标准化

```cpp
// ProvinceUtils::standardize()
新疆维吾尔自治区 → 新疆    广西壮族 → 广西
宁夏回族 → 宁夏            上海/北京/天津/重庆 → 保持
```

计算前批量标准化所有 `order.destinationProvince`，确保 `findPriceRule` 的 QHash 命中率 100%。

### 7. SAX 列过滤器

```cpp
// QXlsx readsax.h — sax_options
const QSet<int> *columnFilter = nullptr;  // 非空时只解析这些列(1-based)

// ExcelEngine.cpp — 根据 columnMapping 构建过滤器
QSet<int> colFilter;
for (auto it = columnMapping.constBegin(); ...) {
    if (it.key().startsWith("__")) continue;
    colFilter.insert(it.value() + 1);
}
opt.columnFilter = &colFilter;
```

---

## 四、常见扩展场景

### 新增快递公司

1. `RuleManager.cpp` → `initAllPriceTables()` 添加 `CourierPrices`
2. `MainWindow.cpp` → `courierCombo` 下拉添加选项
3. `CustomerDialog.cpp` → 快递公司下拉添加选项
4. `MappingTemplate.cpp` → `builtinTemplates()` 添加默认模板

### 新增计算模式

1. `CalculationRule.h` → `Mode` 枚举添加新模式
2. `CalculationRule.cpp` → 实现 `calculateXxx()` + 在 `getCalcFunc` 注册
3. `MainWindow.ui` → `modeCombo` 添加选项

### 新增加价类型

1. `PriceTable.h` → `IncreaseMode` 枚举添加新模式
2. `PriceTable.h` → 实现 `incXxx()` + 在 `getIncreaseFunc` 表注册
3. `GlobalRulesDialog.cpp` → `createModeCombo()` 添加选项

### 新增图表

1. `ChartWidgets.h/cpp` → 新建 QWidget 子类
2. 实现 `paintEvent()`
3. `ChartDialog.cpp` → 实例化

### 修改列映射关键词

1. `ExcelImporter.cpp` → `keywordMap` 编辑关键词列表
2. `MappingTemplate.cpp` → `builtinTemplates()` 编辑模板列名
3. 映射模板首次启动自动创建（`ensureBuiltinsExist()`）

### 修改 UI 样式

1. `resources/style.qss` — 全局样式表（浅色主题，橙色 `#ff7f20`）
2. 预定义按钮选择器：`#primaryButton`、`#secondaryButton`、`#smallButton`、`#dangerButton`
3. 新增对话框用这些选择器即可自动匹配风格

---

## 五、重要注意事项

### 文件编码
项目源文件使用 **GBK/GB2312** 编码。编辑时保持原有编码，`QStringLiteral()` 中的中文在编译时转 UTF-16。

### RC 编译
`CMAKE_RC_COMPILER` 必须指向 Qt 工具链的 64 位 windres。CMakeLists.txt 第 3-8 行已处理。

### Python 脚本
项目根目录下的测试脚本（`test_*.py`）用于验证计算逻辑，不是构建依赖。运行需 Python 3.8+。

### 多线程安全
- `importFilesAsync`：QMap 值拷贝传参，`shared_ptr` 共享只读数据
- `onCalculateClicked`：`QList<OrderData>` 移动语义捕获，线程独立副本
- 合并操作：`QMutexLocker` 保护 `sharedOrders`

### BUG 雷区
- 别在 `signals:` 和 `private:` 之间加 public 方法 → MOC 会误作 signal
- 别用 `QList::erase()` 在循环中逐条删 → O(n²)，用新建列表 O(n)
- 别用 Python 脚本做批量文本替换 → 容易改坏变量名
- 别在 lambda 内 `[&]` 捕获再过 `QtConcurrent::run` 使用 → 确保 waitForFinished

---

## 六、版本号规则

```
v1.2.2 格式: 主.次.修订

CMakeLists.txt:  project(WukongFreight VERSION X.Y.Z)
main.cpp:        setApplicationVersion("X.Y.Z")
app.rc:          FILEVERSION + 3 处版本字符串
installer/setup.iss: AppVersion + OutputBaseFilename
```

打包命令：
```bash
cd installer
"E:/Program Files (x86)/Inno Setup 6/ISCC.exe" setup.iss
```
