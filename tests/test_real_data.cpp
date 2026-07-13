#include <QCoreApplication>
#include <QList>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QTime>
#include <cmath>
#include <cassert>

#include "DataModel/OrderData.h"
#include "DataModel/PriceTable.h"
#include "DataModel/CalculationRule.h"
#include "Calculation/FreightCalculator.h"
#include "Calculation/SimdCalculator.h"
#include "Calculation/RuleManager.h"
#include "Utils/ProvinceUtils.h"
#include "Excel/ExcelEngine.h"

// ============================================================
// 辅助：构建默认报价表（与 RuleManager::initAllPriceTables 一致，申通）
// ============================================================
static QList<PriceRule> buildDefaultPriceTable() {
    QList<PriceRule> table;

    struct ZonePrices { double p05, p1, p2, p3, base3, add3_30, base30, add30; };
    ZonePrices z1 = {2.26,2.46,3.56,4.76,4.76,0.8,3.86,0.8};
    ZonePrices z2 = {2.26,2.46,3.56,4.76,4.76,1.1,4.06,1.3};
    ZonePrices z3 = {2.26,2.46,3.56,4.76,4.76,1.5,4.06,1.6};
    ZonePrices z4 = {2.56,3.56,4.06,5.06,5.06,2.5,4.06,4.3};

    auto buildZone = [](const QString &region, const QStringList &provinces, const ZonePrices &zp) {
        QList<PriceRule> rules;
        for (const QString &prov : provinces) {
            PriceRule rule;
            rule.region = region;
            rule.province = prov;
            rule.segments.append({0, 0.5, zp.p05, 0, false, false});
            rule.segments.append({0.5, 1, zp.p1, 0, false, false});
            rule.segments.append({1, 2, zp.p2, 0, false, false});
            rule.segments.append({2, 3, zp.p3, 0, false, false});
            rule.segments.append({3, 30, zp.base3, zp.add3_30, false, false});
            rule.segments.append({30, -1, zp.base30, zp.add30, false, false});
            rules.append(rule);
        }
        return rules;
    };

    table.append(buildZone("一区", {"江苏", "浙江", "安徽", "上海"}, z1));
    table.append(buildZone("二区", {"山东", "广东", "福建", "北京", "河南", "湖北", "湖南", "江西", "天津", "河北"}, z2));
    table.append(buildZone("三区", {"山西", "广西", "四川", "重庆", "陕西", "贵州", "辽宁", "吉林", "黑龙江", "云南"}, z3));
    table.append(buildZone("四区", {"海南", "甘肃", "青海", "内蒙古", "宁夏"}, z4));

    // 五区：新疆
    {
        PriceRule xj;
        xj.region = "五区"; xj.province = "新疆";
        xj.segments.append({0, 0.5, 10, 0, false, false});
        xj.segments.append({0.5, 1, 13, 0, false, false});
        xj.segments.append({1, 2, 20, 0, false, false});
        xj.segments.append({2, 3, 25, 0, false, false});
        xj.segments.append({3, 30, 25, 15, false, false});
        xj.segments.append({30, -1, 15, 15, false, false});
        table.append(xj);
    }
    // 五区：西藏
    {
        PriceRule xz;
        xz.region = "五区"; xz.province = "西藏";
        xz.segments.append({0, 0.5, 13, 0, false, false});
        xz.segments.append({0.5, 1, 15, 0, false, false});
        xz.segments.append({1, 2, 25, 0, false, false});
        xz.segments.append({2, 3, 30, 0, false, false});
        xz.segments.append({3, 30, 30, 15, false, false});
        xz.segments.append({30, -1, 15, 15, false, false});
        table.append(xz);
    }

    return table;
}

// ============================================================
// 测试框架
// ============================================================
static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_PASS(name) do { \
    g_passCount++; \
    qDebug() << "[PASS]" << name; \
} while(0)

#define TEST_FAIL(name, detail) do { \
    g_failCount++; \
    qDebug() << "[FAIL]" << name << "-" << detail; \
} while(0)

#define ASSERT_EQ_DBL(name, actual, expected, tolerance) do { \
    double _a = (actual), _e = (expected), _t = (tolerance); \
    if (std::abs(_a - _e) <= _t) { \
        TEST_PASS(name); \
    } else { \
        TEST_FAIL(name, QString("expected=%1, actual=%2").arg(_e, 0, 'f', 4).arg(_a, 0, 'f', 4)); \
    } \
} while(0)

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "==========================================";
    qDebug() << "  悟空快递运费计算系统 - 真实数据测试";
    qDebug() << "==========================================";
    qDebug() << "";

    QList<PriceRule> defaultTable = buildDefaultPriceTable();
    CustomerRule rule;
    rule.customerName = "测试客户";
    rule.calculationMode = "实际重量";
    rule.useDefaultPrice = true;
    rule.courier = "申通";

    // ============================================================
    // 测试1: 基础运费计算 — 各区域、各重量段
    // ============================================================
    qDebug() << "--- 测试1: 基础运费计算（标量路径） ---";

    {
        FreightCalculator calc;
        calc.setDefaultPriceTable(defaultTable);
        GlobalRules gr;
        calc.setGlobalRules(gr);

        // 一区(江苏) 0.3kg → 0-0.5段 → 2.26元
        OrderData o;
        o.actualWeight = 0.3;
        o.destinationProvince = "江苏";
        o.businessTime = QDate(2024, 6, 15);
        ASSERT_EQ_DBL("一区0.3kg", calc.calculateFreight(o, rule), 2.26, 0.01);

        // 一区(上海) 0.8kg → 0.5-1段 → 2.46元
        o.actualWeight = 0.8;
        o.destinationProvince = "上海";
        ASSERT_EQ_DBL("一区0.8kg", calc.calculateFreight(o, rule), 2.46, 0.01);

        // 一区(浙江) 1.5kg → 1-2段 → 3.56元
        o.actualWeight = 1.5;
        o.destinationProvince = "浙江省";
        ASSERT_EQ_DBL("一区1.5kg(带省后缀)", calc.calculateFreight(o, rule), 3.56, 0.01);

        // 一区(江苏) 4.5kg → 3-30段 → 4.76 + (4.5-3)*0.8 = 5.96元
        o.actualWeight = 4.5;
        o.destinationProvince = "江苏";
        ASSERT_EQ_DBL("一区4.5kg", calc.calculateFreight(o, rule), 5.96, 0.01);

        // 二区(北京) 10kg → 3-30段 → 4.76 + (10-3)*1.1 = 12.46元
        o.actualWeight = 10.0;
        o.destinationProvince = "北京";
        ASSERT_EQ_DBL("二区10kg", calc.calculateFreight(o, rule), 12.46, 0.01);

        // 三区(四川) 10kg → 3-30段 → 4.76 + (10-3)*1.5 = 15.26元
        o.actualWeight = 10.0;
        o.destinationProvince = "四川";
        ASSERT_EQ_DBL("三区10kg", calc.calculateFreight(o, rule), 15.26, 0.01);

        // 四区(海南) 10kg → 3-30段 → 5.06 + (10-3)*2.5 = 22.56元
        o.actualWeight = 10.0;
        o.destinationProvince = "海南";
        ASSERT_EQ_DBL("四区10kg", calc.calculateFreight(o, rule), 22.56, 0.01);

        // 五区(新疆) 5kg → 3-30段 → 25 + (5-3)*15 = 55元
        o.actualWeight = 5.0;
        o.destinationProvince = "新疆";
        ASSERT_EQ_DBL("五区5kg(新疆)", calc.calculateFreight(o, rule), 55.0, 0.01);

        // 五区(西藏) 5kg → 3-30段 → 30 + (5-3)*15 = 60元
        o.actualWeight = 5.0;
        o.destinationProvince = "西藏";
        ASSERT_EQ_DBL("五区5kg(西藏)", calc.calculateFreight(o, rule), 60.0, 0.01);

        // 30kg以上 — 一区 35kg → 30+段(firstWeight=3.0) → 3.86 + (35-3)*0.8 = 29.46元
        o.actualWeight = 35.0;
        o.destinationProvince = "江苏";
        ASSERT_EQ_DBL("一区35kg(大货)", calc.calculateFreight(o, rule), 29.46, 0.01);
    }

    // ============================================================
    // 测试2: 无重量订单 + noWeightDefaultPrice + 加价
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试2: 无重量订单 + noWeightDefaultPrice + 加价 ---";

    {
        FreightCalculator calc;
        calc.setDefaultPriceTable(defaultTable);

        GlobalRules gr;
        gr.noWeightDefaultPrice = 5.0;

        // 活动加价：+10% (在日期范围内)
        PriceIncreaseRule activity;
        activity.name = "双11";
        activity.mode = IncreaseMode::PerTicketPercent;
        activity.amount = 0.1;
        activity.startTime = QDateTime(QDate(2024, 11, 1), QTime(0, 0));
        activity.endTime = QDateTime(QDate(2024, 11, 15), QTime(23, 59));
        activity.isActive = true;
        gr.activityRules.append(activity);

        // 省份加价：新疆 +3元(固定)
        PriceIncreaseRule provInc;
        provInc.name = "偏远地区";
        provInc.mode = IncreaseMode::PerTicketFixed;
        provInc.amount = 3.0;
        provInc.province = "新疆";
        provInc.isActive = true;
        gr.provincePriceIncreases.append(provInc);

        calc.setGlobalRules(gr);

        // 无重量订单，普通省份（江苏），在活动期间 → 5.0 * 1.1 = 5.5
        OrderData o;
        o.actualWeight = 0;
        o.destinationProvince = "江苏";
        o.businessTime = QDate(2024, 11, 10);
        ASSERT_EQ_DBL("无重量+活动加价(江苏)", calc.calculateFreight(o, rule), 5.5, 0.01);

        // 无重量订单，新疆，在活动期间 → 5.0 * 1.1 + 3.0 = 8.5
        o.destinationProvince = "新疆";
        ASSERT_EQ_DBL("无重量+活动+省份加价(新疆)", calc.calculateFreight(o, rule), 8.5, 0.01);

        // 无重量订单，不在活动期间 → 5.0
        o.businessTime = QDate(2024, 12, 1);
        o.destinationProvince = "江苏";
        ASSERT_EQ_DBL("无重量无活动(江苏)", calc.calculateFreight(o, rule), 5.0, 0.01);

        // 无重量订单，新疆，不在活动期间 → 5.0 + 3.0 = 8.0
        o.destinationProvince = "新疆";
        ASSERT_EQ_DBL("无重量+省份加价(新疆)", calc.calculateFreight(o, rule), 8.0, 0.01);
    }

    // ============================================================
    // 测试3: 省份名标准化（带后缀的省份应正确匹配）
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试3: 省份名标准化 ---";

    {
        FreightCalculator calc;
        calc.setDefaultPriceTable(defaultTable);
        calc.setGlobalRules(GlobalRules());

        OrderData o;
        o.actualWeight = 1.5;
        o.businessTime = QDate(2024, 6, 15);

        // 测试各种省份后缀
        struct { QString input; QString expected; } cases[] = {
            {"江苏省", "江苏"},
            {"上海市", "上海"},
            {"浙江省", "浙江"},
            {"内蒙古自治区", "内蒙古"},
            {"新疆维吾尔自治区", "新疆"},
            {"西藏自治区", "西藏"},
            {"广西壮族自治区", "广西"},
            {"宁夏回族自治区", "宁夏"},
        };

        for (const auto &c : cases) {
            o.destinationProvince = c.input;
            double freight = calc.calculateFreight(o, rule);
            // 与标准省份名比较
            o.destinationProvince = c.expected;
            double freightStd = calc.calculateFreight(o, rule);
            ASSERT_EQ_DBL(QString("省份标准化: %1 vs %2").arg(c.input).arg(c.expected),
                          freight, freightStd, 0.001);
        }
    }

    // ============================================================
    // 测试4: 标量路径 vs SIMD路径 一致性
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试4: 标量路径 vs SIMD路径 一致性 ---";

    {
        FreightCalculator calc;
        calc.setDefaultPriceTable(defaultTable);

        GlobalRules gr;
        gr.noWeightDefaultPrice = 5.0;

        // 活动加价
        PriceIncreaseRule activity;
        activity.name = "618";
        activity.mode = IncreaseMode::PerTicketPercent;
        activity.amount = 0.05;
        activity.startTime = QDateTime(QDate(2024, 6, 1), QTime(0, 0));
        activity.endTime = QDateTime(QDate(2024, 6, 20), QTime(23, 59));
        activity.isActive = true;
        gr.activityRules.append(activity);

        // 临时加价
        PriceIncreaseRule temp;
        temp.name = "油价调整";
        temp.mode = IncreaseMode::PerTicketFixed;
        temp.amount = 0.5;
        temp.startTime = QDateTime(QDate(2024, 6, 10), QTime(0, 0));
        temp.endTime = QDateTime(QDate(2024, 6, 30), QTime(23, 59));
        temp.isActive = true;
        gr.tempPriceIncreases.append(temp);

        // 省份加价
        PriceIncreaseRule provInc;
        provInc.name = "偏远";
        provInc.mode = IncreaseMode::PerKg;
        provInc.amount = 0.3;
        provInc.province = "新疆";
        provInc.isActive = true;
        gr.provincePriceIncreases.append(provInc);

        calc.setGlobalRules(gr);

        // 创建测试订单
        QStringList provinces = {"江苏", "上海", "北京", "广东", "四川", "海南", "新疆", "西藏", "浙江", "山东"};
        QList<OrderData> orders;

        int seed = 42;
        auto randWeight = [&seed]() -> double {
            seed = seed * 1103515245 + 12345;
            int r = (seed / 65536) % 1000;
            // 0~50kg, 含0重量
            if (r < 50) return 0.0;  // 5% 无重量
            return (r / 10.0);       // 0.1~99.9kg
        };

        for (int i = 0; i < 1000; ++i) {
            OrderData o;
            o.waybillNo = QString("YT%1").arg(i + 1, 6, 10, QChar('0'));
            o.actualWeight = randWeight();
            o.volumetricWeight = 0;
            o.destinationProvince = provinces[i % provinces.size()];
            o.businessTime = QDate(2024, 6, 15);  // 在活动+临时加价范围内
            o.customer = "测试客户";
            orders.append(o);
        }

        // 标量路径
        QList<OrderData> scalarOrders = orders;
        calc.calculateBatch(scalarOrders, rule, 4);

        // SIMD路径
        QList<OrderData> simdOrders = orders;
        SimdCalculator::calculateChunk(simdOrders.data(), simdOrders.size(), rule, defaultTable, gr);

        // 比较结果
        int mismatchCount = 0;
        double maxDiff = 0;
        for (int i = 0; i < orders.size(); ++i) {
            double diff = std::abs(scalarOrders[i].freight - simdOrders[i].freight);
            if (diff > maxDiff) maxDiff = diff;
            if (diff > 0.02) {
                mismatchCount++;
                if (mismatchCount <= 5) {
                    qDebug() << "  [MISMATCH] #" << i
                             << "prov=" << orders[i].destinationProvince
                             << "wt=" << orders[i].actualWeight
                             << "scalar=" << scalarOrders[i].freight
                             << "simd=" << simdOrders[i].freight
                             << "diff=" << diff;
                }
            }
        }

        if (mismatchCount == 0) {
            TEST_PASS("标量vs SIMD 一致性 (1000条)");
        } else {
            TEST_FAIL("标量vs SIMD 一致性", QString("mismatch=%1, maxDiff=%2").arg(mismatchCount).arg(maxDiff, 0, 'f', 6));
        }
        qDebug() << "  最大差异:" << maxDiff;
    }

    // ============================================================
    // 测试5: 线程安全测试（多线程批量计算不崩溃）
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试5: 线程安全测试 ---";

    {
        FreightCalculator calc;
        calc.setDefaultPriceTable(defaultTable);
        calc.setGlobalRules(GlobalRules());

        QList<OrderData> orders;
        QStringList provinces = {"江苏", "上海", "北京", "广东", "四川", "海南", "新疆", "西藏"};
        for (int i = 0; i < 50000; ++i) {
            OrderData o;
            o.actualWeight = 0.1 + (i % 500) / 10.0;
            o.destinationProvince = provinces[i % provinces.size()];
            o.businessTime = QDate(2024, 6, 15);
            orders.append(o);
        }

        // 多线程批量计算
        calc.calculateBatch(orders, rule, 8);

        // 验证所有订单都有有效结果
        int errorCount = 0;
        for (const OrderData &o : orders) {
            if (!o.isValid || o.freight <= 0) errorCount++;
        }

        if (errorCount == 0) {
            TEST_PASS("线程安全 (50000条, 8线程)");
        } else {
            TEST_FAIL("线程安全", QString("errorCount=%1").arg(errorCount));
        }
    }

    // ============================================================
    // 测试6: Excel 导入导出 round-trip
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试6: Excel 导入导出 round-trip ---";

    {
        // 创建测试订单
        QList<OrderData> orders;
        for (int i = 0; i < 100; ++i) {
            OrderData o;
            o.waybillNo = QString("SF%1").arg(i + 1, 6, 10, QChar('0'));
            o.actualWeight = 0.5 + i * 0.3;
            o.volumetricWeight = 0;
            o.destinationProvince = QStringList{"江苏","上海","北京","广东","四川"}[i % 5];
            o.businessTime = QDate(2024, 6, 15);
            o.customer = "测试客户";
            o.client = "客户A";
            o.freight = 5.0 + i * 0.5;
            o.usedRule = "默认报价-江苏(一区)";
            o.isValid = true;
            orders.append(o);
        }

        QStringList headers = {"运单号", "业务时间", "体积重", "订单客户", "客户",
                               "目的省份", "实际重量", "运费", "计算规则和策略"};

        // 使用公开接口导出（<50000条用QXlsx路径）
        ExcelEngine engine;
        QString testFile = "/tmp/wukong_test_export.xlsx";

        bool exportOk = engine.writeExcel(testFile, orders, headers);
        if (exportOk) {
            TEST_PASS("Excel 快速导出");

            // 验证文件存在且非空
            QFile f(testFile);
            if (f.exists() && f.size() > 0) {
                TEST_PASS("导出文件存在且非空");

                // 重新导入
                QStringList importedHeaders = engine.getHeaders(testFile);
                if (importedHeaders.size() == headers.size()) {
                    TEST_PASS("表头列数一致");
                } else {
                    TEST_FAIL("表头列数一致",
                        QString("expected=%1, actual=%2").arg(headers.size()).arg(importedHeaders.size()));
                }

                // 验证表头内容
                bool headersMatch = true;
                for (int i = 0; i < qMin(headers.size(), importedHeaders.size()); ++i) {
                    if (headers[i] != importedHeaders[i]) {
                        headersMatch = false;
                        qDebug() << "  表头不匹配 [" << i << "]:" << headers[i] << "vs" << importedHeaders[i];
                        break;
                    }
                }
                if (headersMatch) TEST_PASS("表头内容匹配");
                else TEST_FAIL("表头内容匹配", "内容不一致");

            } else {
                TEST_FAIL("导出文件存在且非空", "文件不存在或为空");
            }
        } else {
            TEST_FAIL("Excel 快速导出", "导出失败");
        }
    }

    // ============================================================
    // 测试7: 空表头列错位修复验证
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试7: 空表头列错位修复 ---";

    {
        // 创建一个有空白列表头的Excel文件，>50000条触发 writeExcelFast 路径
        QList<OrderData> orders;
        OrderData o;
        o.actualWeight = 1.5;
        o.destinationProvince = "江苏";
        o.businessTime = QDate(2024, 6, 15);
        o.customer = "客户A";
        o.client = "客户A";
        o.freight = 3.56;
        o.usedRule = "默认报价-江苏";
        o.isValid = true;
        for (int i = 0; i < 50001; ++i) {
            o.waybillNo = QString("T%1").arg(i);
            orders.append(o);
        }

        // 表头中第3列(索引2)为空
        QStringList headers = {"运单号", "业务时间", "", "订单客户", "客户",
                               "目的省份", "实际重量", "运费", "计算规则和策略"};

        ExcelEngine engine;
        QString testFile = "/tmp/wukong_test_empty_header.xlsx";
        bool exportOk = engine.writeExcel(testFile, orders, headers);

        if (exportOk) {
            QStringList importedHeaders = engine.getHeaders(testFile);
            // 修复前：空表头被跳过，列数=8（少了1个）
            // 修复后：空表头保留，列数=9
            if (importedHeaders.size() == headers.size()) {
                TEST_PASS("空表头列数正确");
            } else {
                TEST_FAIL("空表头列数正确",
                    QString("expected=%1, actual=%2").arg(headers.size()).arg(importedHeaders.size()));
            }
        } else {
            TEST_FAIL("空表头导出", "导出失败");
        }
    }

    // ============================================================
    // 测试8: XML 双重转义修复验证
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试8: XML 双重转义修复 ---";

    {
        // 创建包含特殊字符的订单，>50000条触发 writeExcelFast 路径
        QList<OrderData> orders;
        OrderData o;
        o.actualWeight = 1.5;
        o.destinationProvince = "江苏";
        o.businessTime = QDate(2024, 6, 15);
        o.customer = "客户&<>A";
        o.client = "客户&<>A";
        o.freight = 3.56;
        o.usedRule = "规则&<>测试";
        o.isValid = true;
        for (int i = 0; i < 50001; ++i) {
            o.waybillNo = QString("SF%1").arg(i);
            orders.append(o);
        }

        QStringList headers = {"运单号", "业务时间", "体积重", "订单客户", "客户",
                               "目的省份", "实际重量", "运费", "计算规则和策略"};

        ExcelEngine engine;
        QString testFile = "/tmp/wukong_test_xml_escape.xlsx";
        bool exportOk = engine.writeExcel(testFile, orders, headers);

        if (exportOk) {
            // 读取文件内容检查是否有双重转义
            QFile f(testFile);
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray content = f.readAll();
                f.close();

                // 检查是否有 &amp;amp; 这样的双重转义
                if (!content.contains("&amp;amp;") && !content.contains("&amp;lt;") && !content.contains("&amp;gt;")) {
                    TEST_PASS("XML 无双重转义");
                } else {
                    TEST_FAIL("XML 无双重转义", "发现双重转义序列");
                }
            } else {
                TEST_FAIL("XML 无双重转义", "无法读取文件");
            }
        } else {
            TEST_FAIL("XML 双重转义导出", "导出失败");
        }
    }

    // ============================================================
    // 测试9: ProvinceUtils 空输入安全
    // ============================================================
    qDebug() << "";
    qDebug() << "--- 测试9: ProvinceUtils 空输入安全 ---";

    {
        // 空输入不应匹配
        if (!ProvinceUtils::matches("", "江苏") && !ProvinceUtils::matches("江苏", "")) {
            TEST_PASS("空输入不匹配");
        } else {
            TEST_FAIL("空输入不匹配", "空字符串被错误匹配");
        }

        // 正常匹配仍然有效
        if (ProvinceUtils::matches("江苏", "江苏")) {
            TEST_PASS("正常匹配有效");
        } else {
            TEST_FAIL("正常匹配有效", "正常匹配失败");
        }
    }

    // ============================================================
    // 汇总
    // ============================================================
    qDebug() << "";
    qDebug() << "==========================================";
    qDebug() << "  测试结果汇总";
    qDebug() << "==========================================";
    qDebug() << "  通过:" << g_passCount;
    qDebug() << "  失败:" << g_failCount;
    qDebug() << "  总计:" << (g_passCount + g_failCount);
    qDebug() << "==========================================";

    return g_failCount > 0 ? 1 : 0;
}
