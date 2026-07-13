# 悟空结算 — 优化改进建议

基于完整代码审查，按优先级和实现难度分类。

---

## 一、体验优化（P0 — 财务人员每天用到的）

### 1. 登录记住密码
- **现状**: 每次打开都要输入密码
- **建议**: 加"记住密码"复选框，加密存储到本地
- **改动**: [LoginDialog](src/Auth/LoginDialog.cpp) + `QSettings`

### 2. 数据表格支持双击编辑
- **现状**: 导入后表格只读，一个省份名写错要回到 Excel 改完重新导入
- **建议**: 表格允许双击编辑，改完点"重新计算"
- **改动**: [MainWindow::displayPage](src/MainWindow.cpp) `setEditTriggers` 改为 `DoubleClicked`

### 3. 异常行高亮
- **现状**: 错误信息在最后一列，表宽看不到
- **建议**: 有错误信息的行标红底色，顶部加"异常: 12 条"按钮点一下只看异常
- **改动**: [displayPage](src/MainWindow.cpp) 遍历时检查 `order.errorMessage`

### 4. 导出按客户分 Sheet + 汇总行
- **现状**: 导出只有"合并导出"和"按客户分别导出"两种，但"按客户分别导出"是独立文件
- **建议**: 单个 xlsx 内按客户分 Sheet，每个 Sheet 底部加汇总行（件数/运费/均重合计）
- **改动**: [ExcelExporter](src/Excel/ExcelExporter.cpp)

### 5. 图表标签截断
- **现状**: 店铺名太长，柱形图下面重叠
- **建议**: 超过 6 个字截断加"…"，或改用横向条形图
- **改动**: [ChartDialog](src/UI/ChartDialog.cpp) + [ChartWidgets](src/UI/ChartWidgets.cpp)

---

## 二、性能优化（P1 — 300 万行场景）

### 6. 省份规则查表缓存
- **现状**: 每次计算都遍历全部省份加价规则列表
- **建议**: 启动时构建 `QHash<省份, PriceIncreaseRule>`，O(1) 查找
- **改动**: [FreightCalculator::buildRuleCache](src/Calculation/FreightCalculator.cpp)

### 7. 活动/临时加价规则也做缓存
- **现状**: 每条订单都遍历全部活动规则+临时规则列表（即使列表只有几条）
- **建议**: 计算前筛选出时间范围内的规则存到临时列表，避免每条都做 `isTimeInRange`
- **改动**: [SimdCalculator](src/Calculation/SimdCalculator.cpp) Phase 6 前预筛选

### 8. 内存预估保护
- **现状**: `allOrders.reserve(total + 1000)` 对大文件无上限
- **建议**: 预估超过 500 万行时分批计算，避免内存 OOM
- **改动**: [MainWindow::importFilesAsync](src/MainWindow.cpp)

### 9. 启动时预加载规则
- **现状**: 每次计算时从 `m_ruleManager` 取规则，如果在计算过程中有人改了规则（虽然 UI 上禁用了按钮），可能不一致
- **建议**: `onCalculateClicked` 时深拷贝一份 `GlobalRules` 传给计算线程
- **现状**: 已经这么做了 ✅（line 957 `GlobalRules globalRules = m_calculator->globalRules()`）

---

## 三、架构改进（P2 — 长远维护）

### 10. 提取计算管道（Pipeline Pattern）
- **现状**: `calculateFreightDetail` 和 `SimdCalculator` 各自实现相同的步骤（匹配报价→加权段→活动→临时→省份→拉均重），逻辑重复
- **建议**: 设计一个 `CalculationPipeline` 类，包含有序的步骤列表，统一执行
```cpp
Pipeline pipeline;
pipeline.addStep(new WeightSegmentMatcher());
pipeline.addStep(new ActivityIncreaseApplier());
pipeline.addStep(new TempIncreaseApplier());
pipeline.addStep(new ProvinceIncreaseApplier());
pipeline.addStep(new AvgWeightSurchargeApplier());
OrderData result = pipeline.execute(order, rule, globalRules);
```

### 11. 报价表校验
- **现状**: 报价表数据硬编码在 `RuleManager::initAllPriceTables()`，无校验
- **建议**: 加载时检查重量段是否覆盖 0~∞、首重价是否单调递增
- **改动**: [RuleManager](src/Calculation/RuleManager.cpp)

### 12. 计算结果的确定性验证
- **现状**: 无任何回归测试
- **建议**: 准备一组标准测试数据（已知重量、省份、预期运费），每次改动后跑一遍
- **改动**: 新增 `test_regression.cpp` 或 Python 测试脚本

---

## 四、功能扩展（未来方向）

### 13. 多月份对比图表
- 场景: 对比 1 月和 2 月各店铺运费变化
- 改动: [ChartDialog](src/UI/ChartDialog.cpp) 支持加载多个历史记录

### 14. 运费差异预警
- 场景: 同一客户同一省份，本月运费比上月涨了 20%，弹出预警
- 改动: [CalculationHistory](src/DataModel/CalculationHistory.h) 对比逻辑

### 15. 一键对账
- 场景: 导入快递公司的账单（含"快递公司收费"列），跟自己的计算结果对比，标出差异
- 改动: [FreightCalculator](src/Calculation/FreightCalculator.cpp) + 新增列"差异"

### 16. 报表打印/PDF 导出
- 场景: 给客户签字确认、给老板看月报
- 改动: 引入 `QPrinter` + `QPainter` 生成 PDF

---

## 五、代码质量

| # | 问题 | 位置 |
|---|---|---|
| 17 | 多处 `QStringLiteral` 和 `QString::fromUtf8` 混用，编码不一致 | 全局 |
| 18 | `m_isCalculating` 无超时保护——如果计算线程卡死，按钮永远禁用 | [MainWindow](src/MainWindow.cpp) |
| 19 | `displayPage` 每页都 new `QStandardItemModel`，翻页频繁时内存碎片 | [MainWindow](src/MainWindow.cpp) |
| 20 | Excel 导入时 `QThreadPool::globalInstance()` 被修改（已修复），但 `QtConcurrent::run` 无异常处理 | [MainWindow](src/MainWindow.cpp) |

---

## 优先级排序

| 优先级 | 编号 | 预期收益 | 实现难度 |
|---|---|---|---|
| 🔴 立即 | 1. 记住密码 | 每次省 5 秒 | 低 |
| 🔴 立即 | 2. 双击编辑 | 不用回 Excel 改 | 低 |
| 🔴 立即 | 3. 异常高亮 | 不会漏看错误 | 低 |
| 🔴 立即 | 5. 标签截断 | 图表可用 | 低 |
| 🟡 本周 | 4. 导出分 Sheet | 月结刚需 | 中 |
| 🟡 本周 | 6/7. 规则缓存 | 300万行快 30% | 中 |
| 🟢 有空 | 10. 计算管道 | 维护性 | 高 |
| 🟢 有空 | 12. 回归测试 | 避免改坏 | 中 |
| 🔵 未来 | 13-16. 功能扩展 | 竞争力 | 高 |
