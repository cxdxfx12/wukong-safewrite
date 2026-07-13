#!/usr/bin/env python3
"""
测试加价规则逻辑 — 模拟 C++ 端 FreightCalculator + CalculationRule 行为
每一组测试都标注了对应的 C++ 代码位置
"""
import math, sys
from datetime import datetime, timedelta
from enum import IntEnum

# ====== 模拟 C++ 数据结构 ======

class IncreaseMode(IntEnum):
    PerTicketFixed = 0    # 单票固定加价
    PerTicketPercent = 1  # 单票加百分比
    PerKg = 2             # 每KG加价

class PriceIncreaseRule:
    def __init__(self, name, mode, amount, province="", start=None, end=None, active=True):
        self.name = name
        self.mode = mode
        self.amount = amount
        self.province = province
        self.start = start
        self.end = end
        self.active = active

    def is_time_in_range(self, dt):
        if not self.active: return False
        if self.start and dt < self.start: return False
        if self.end and dt > self.end: return False
        return True

# ====== 模拟 applyPriceIncreases (CalculationRule.cpp) ======

def apply_price_increases(freight, weight, rules, time_filter=None):
    """统一加价函数 — 对应 CalculationRule::applyPriceIncreases"""
    result = freight
    for rule in rules:
        if not rule.active:
            continue
        if time_filter is not None and not rule.is_time_in_range(time_filter):
            continue
        if rule.mode == IncreaseMode.PerTicketFixed:
            result += rule.amount
        elif rule.mode == IncreaseMode.PerTicketPercent:
            result *= (1.0 + rule.amount)
        elif rule.mode == IncreaseMode.PerKg:
            result += weight * rule.amount
    return result

# ====== 模拟省份匹配逻辑 (FreightCalculator.cpp / SimdCalculator.cpp) ======

def normalize_province(p):
    for sf in ["省", "市", "自治区", "特别行政区"]:
        if p.endswith(sf):
            p = p[:-len(sf)]
            break
    return p.strip()

def province_matches(order_prov, rule_prov):
    """三重匹配"""
    np = normalize_province(order_prov)
    rp = normalize_province(rule_prov)
    return (rp == np or rule_prov == order_prov
            or np.find(rp) >= 0 or rp.find(np) >= 0
            or (len(np) >= 2 and len(rp) >= 2 and np[:2] == rp[:2]))

def apply_province_increases(freight, weight, rules, order_province):
    """省份加价 — 对应 FreightCalculator.cpp 修复后的 inline 逻辑"""
    result = freight
    for rule in rules:
        if not rule.active or not rule.province:
            continue
        if not province_matches(order_province, rule.province):
            continue
        if rule.mode == IncreaseMode.PerTicketFixed:
            result += rule.amount
        elif rule.mode == IncreaseMode.PerTicketPercent:
            result *= (1.0 + rule.amount)
        elif rule.mode == IncreaseMode.PerKg:
            result += weight * rule.amount
    return result

# ====== 模拟基础运费 ======

def calculate_base_freight(weight):
    """模拟申通一区 3-30kg 段: 首重 4.76 + 续重 0.8/kg"""
    if weight <= 3:
        return 4.76
    return 4.76 + (weight - 3) * 0.8

# ====== 测试 ======

passed = 0
failed = 0

def test(name, actual, expected):
    global passed, failed
    ok = abs(actual - expected) < 0.01
    status = "✅" if ok else "❌"
    print(f"  {status} {name}: {actual:.2f} (期望 {expected:.2f})")
    if ok:
        passed += 1
    else:
        failed += 1

print("=" * 60)
print("测试 1: 基础运费（无加价规则）")
print("=" * 60)
base = calculate_base_freight(5.0)  # 5kg
test("5kg 申通一区 首重4.76+续重0.8*2", base, 4.76 + 2.0 * 0.8)  # = 6.36

print("\n" + "=" * 60)
print("测试 2: 活动加价 — 三种模式")
print("=" * 60)
now = datetime.now()
activity_rules = [
    PriceIncreaseRule("双11固定", IncreaseMode.PerTicketFixed, 2.0, start=now, end=now+timedelta(days=1)),
    PriceIncreaseRule("春节百分比", IncreaseMode.PerTicketPercent, 0.1, start=now, end=now+timedelta(days=1)),
    PriceIncreaseRule("重货加价", IncreaseMode.PerKg, 0.5, start=now, end=now+timedelta(days=1)),
]
f = apply_price_increases(base, 5.0, activity_rules, time_filter=now)
# 固定+2 → 百分比×1.1 → 每KG+0.5×5=2.5
# 6.36+2=8.36 → 8.36*1.1=9.196 → 9.196+2.5=11.696
test("活动加价组合(固定+2,百分比+10%,每KG+0.5×5kg)", f, 11.70)

print("\n" + "=" * 60)
print("测试 3: 时间范围过滤 — 过期规则不生效")
print("=" * 60)
old_rules = [
    PriceIncreaseRule("过期活动", IncreaseMode.PerTicketFixed, 100.0,
                      start=now-timedelta(days=10), end=now-timedelta(days=1)),
]
f = apply_price_increases(base, 5.0, old_rules, time_filter=now)
test("过期活动不生效", f, base)

print("\n" + "=" * 60)
print("测试 4: 未启用规则不生效")
print("=" * 60)
inactive_rules = [
    PriceIncreaseRule("停用规则", IncreaseMode.PerTicketFixed, 100.0,
                      start=now, end=now+timedelta(days=1), active=False),
]
f = apply_price_increases(base, 5.0, inactive_rules, time_filter=now)
test("停用规则不生效", f, base)

print("\n" + "=" * 60)
print("测试 5: 省份加价 — 关键！仅匹配省份生效")
print("=" * 60)
province_rules = [
    PriceIncreaseRule("新疆偏远", IncreaseMode.PerTicketFixed, 5.0, province="新疆"),
    PriceIncreaseRule("西藏偏远", IncreaseMode.PerTicketFixed, 8.0, province="西藏"),
    PriceIncreaseRule("上海加价", IncreaseMode.PerKg, 1.0, province="上海"),
]

# 新疆订单 — 应该只加5元
f_xj = apply_province_increases(base, 5.0, province_rules, "新疆维吾尔自治区")
test("新疆订单 → 仅+5元", f_xj, base + 5.0)

# 上海订单 — 应该只加 1.0×5=5元
f_sh = apply_province_increases(base, 5.0, province_rules, "上海市")
test("上海订单 → 仅+5元(每KG×5)", f_sh, base + 5.0)

# 江苏订单 — 应该不加价
f_js = apply_province_increases(base, 5.0, province_rules, "江苏")
test("江苏订单 → 不加价", f_js, base)

# 浙江订单 — 应该不加价
f_zj = apply_province_increases(base, 5.0, province_rules, "浙江省")
test("浙江订单 → 不加价", f_zj, base)

print("\n" + "=" * 60)
print("测试 6: 省份模糊匹配")
print("=" * 60)
fuzzy_rules = [
    PriceIncreaseRule("新疆加价", IncreaseMode.PerTicketFixed, 5.0, province="新疆"),
]
# "新疆" vs "新疆维吾尔自治区" → 前两字"新疆"匹配
f1 = apply_province_increases(base, 5.0, fuzzy_rules, "新疆维吾尔自治区")
test("'新疆' 匹配 '新疆维吾尔自治区'", f1, base + 5.0)

# "新疆" vs "新疆" → 精确匹配
f2 = apply_province_increases(base, 5.0, fuzzy_rules, "新疆")
test("'新疆' 匹配 '新疆'", f2, base + 5.0)

print("\n" + "=" * 60)
print("测试 7: 完整链路 — 基础运费 + 活动 + 临时 + 省份")
print("=" * 60)
# 模拟一次完整的 calculateFreightDetail 调用
weight = 8.0       # 8kg
province = "新疆维吾尔自治区"
order_date = now

base_freight = calculate_base_freight(weight)  # 4.76 + 5*0.8 = 8.76

# 活动加价（时间范围内）
act_rules = [PriceIncreaseRule("双11", IncreaseMode.PerTicketPercent, 0.05,  # +5%
                                start=now, end=now+timedelta(days=1))]
# 临时加价（时间范围内）
tmp_rules = [PriceIncreaseRule("油价涨", IncreaseMode.PerTicketFixed, 1.5,
                                start=now, end=now+timedelta(days=1))]
# 省份加价（仅新疆）
prv_rules = [PriceIncreaseRule("新疆", IncreaseMode.PerTicketFixed, 5.0, province="新疆"),
             PriceIncreaseRule("西藏", IncreaseMode.PerTicketFixed, 8.0, province="西藏")]

result = base_freight
result = apply_price_increases(result, weight, act_rules, time_filter=order_date)
result = apply_price_increases(result, weight, tmp_rules, time_filter=order_date)
result = apply_province_increases(result, weight, prv_rules, province)

# 8.76 → ×1.05 = 9.198 → +1.5 = 10.698 → +5.0(新疆) = 15.698
expected = ((base_freight * 1.05) + 1.5) + 5.0
test("完整链路: 基础8.76→+5%→+1.5→+5(新疆)=15.70", result, expected)

print("\n" + "=" * 60)
print("测试 8: 省份不匹配 — 不应加价")
print("=" * 60)
result2 = base_freight
result2 = apply_price_increases(result2, weight, act_rules, time_filter=order_date)
result2 = apply_price_increases(result2, weight, tmp_rules, time_filter=order_date)
result2 = apply_province_increases(result2, weight, prv_rules, "江苏省")
expected2 = (base_freight * 1.05) + 1.5  # 不加省份
test("江苏订单: 基础8.76→+5%→+1.5→+0(无省份)=10.70", result2, expected2)

# ====== 结果 ======
print("\n" + "=" * 60)
print(f"  总计: {passed+failed} 个测试 | 通过: {passed} | 失败: {failed}")
print("=" * 60)
sys.exit(0 if failed == 0 else 1)
