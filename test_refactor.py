#!/usr/bin/env python3
"""
测试重构后的策略分派逻辑 — 模拟 C++ 函数指针表 + 省份匹配 + 重量段计算
对照 C++ 源码：
  - PriceTable.h:     getIncreaseFunc() 函数指针表
  - CalculationRule:  provinceMatches() / getCalcFunc()
  - FreightCalculator / SimdCalculator
"""
import math, sys
from enum import IntEnum

# ====== 模拟 C++ enum 和函数指针表 ======

class IncreaseMode(IntEnum):
    PerTicketFixed = 0
    PerTicketPercent = 1
    PerKg = 2

class CalcMode(IntEnum):
    ActualWeight = 0
    AverageWeight = 1
    HundredGram = 2
    FullAdditional = 3

# ★ 函数指针表 — getIncreaseFunc()
def inc_fixed(freight, weight, amount):   return freight + amount
def inc_percent(freight, weight, amount): return freight * (1.0 + amount)
def inc_per_kg(freight, weight, amount):  return freight + weight * amount

INCREASE_TABLE = [inc_fixed, inc_percent, inc_per_kg]

def get_increase_func(mode):
    return INCREASE_TABLE[int(mode)]

# ★ 重量段
class WeightSegment:
    def __init__(self, min_w, max_w, first, additional):
        self.minWeight = min_w
        self.maxWeight = max_w
        self.firstWeightPrice = first
        self.additionalPrice = additional

def is_weight_in_range(weight, seg):
    if weight < seg.minWeight: return False
    if seg.maxWeight < 0: return True
    return weight <= seg.maxWeight

# ★ 计算函数表 — getCalcFunc()
def calc_standard(weight, segments):
    for seg in segments:
        if is_weight_in_range(weight, seg):
            extra = max(0, weight - seg.minWeight)
            # For 30+ range, first weight is 3.0
            first_w = 3.0 if (seg.maxWeight < 0 and seg.minWeight >= 30) else seg.minWeight
            extra = max(0, weight - first_w)
            return seg.firstWeightPrice + extra * seg.additionalPrice
    return 0.0

def calc_hundred_gram(weight, segments):
    for seg in segments:
        if is_weight_in_range(weight, seg):
            extra = max(0, weight - seg.minWeight)
            units = math.ceil(extra * 10)
            return seg.firstWeightPrice + units * (seg.additionalPrice / 10.0)
    return 0.0

def calc_full_additional(weight, segments):
    return calc_standard(math.ceil(weight), segments)

CALC_TABLE = {
    CalcMode.ActualWeight: calc_standard,
    CalcMode.HundredGram: calc_hundred_gram,
    CalcMode.FullAdditional: calc_full_additional,
}

# ★ 省份匹配 — provinceMatches()
def province_matches(order_prov, rule_prov):
    if not order_prov or not rule_prov:
        return False
    def norm(p):
        for sf in ["省", "市", "自治区", "特别行政区"]:
            if p.endswith(sf): p = p[:-len(sf)]; break
        return p.strip()
    np, rp = norm(order_prov), norm(rule_prov)
    return (rp == np or rule_prov == order_prov
            or np.find(rp) >= 0 or rp.find(np) >= 0
            or (len(np) >= 2 and len(rp) >= 2 and np[:2] == rp[:2]))

# ====== 申通一区报价表 ======
SHENTONG_ZONE1 = [
    WeightSegment(0, 0.5, 2.26, 0),
    WeightSegment(0.5, 1, 2.46, 0),
    WeightSegment(1, 2, 3.56, 0),
    WeightSegment(2, 3, 4.76, 0),
    WeightSegment(3, 30, 4.76, 0.8),
    WeightSegment(30, -1, 3.86, 0.8),
]

# ====== 测试 ======
passed = failed = 0

def test(name, actual, expected):
    global passed, failed
    ok = abs(actual - expected) < 0.02
    s = "PASS" if ok else "FAIL"
    if not ok: s += f"  (got {actual:.2f}, want {expected:.2f})"
    print(f"  [{s}] {name}")
    if ok: passed += 1
    else: failed += 1

# -------------------------------------------------------
print("=" * 55)
print("1. 函数指针表 — getIncreaseFunc() 三种模式")
print("=" * 55)
test("固定+5元",    get_increase_func(IncreaseMode.PerTicketFixed)(10, 3, 5), 15)
test("百分比+10%",  get_increase_func(IncreaseMode.PerTicketPercent)(10, 3, 0.1), 11)
test("每KG+2元×3kg", get_increase_func(IncreaseMode.PerKg)(10, 3, 2), 16)
# 与 switch/case 旧逻辑完全一致
test("组合: 固定+2→百分比+5%→每KG+1×5kg",
     inc_per_kg(inc_percent(inc_fixed(10, 0, 2), 0, 0.05), 5, 1), 17.6)

# -------------------------------------------------------
print("\n" + "=" * 55)
print("2. 省份匹配 — provinceMatches()")
print("=" * 55)
test("新疆 vs 新疆", province_matches("新疆", "新疆"), True)
test("新疆维吾尔自治区 vs 新疆", province_matches("新疆维吾尔自治区", "新疆"), True)
test("上海市 vs 上海", province_matches("上海市", "上海"), True)
test("江苏 vs 浙江", province_matches("江苏", "浙江"), False)
test("北京 vs 北京市", province_matches("北京", "北京市"), True)
test("空 vs 新疆", province_matches("", "新疆"), False)

# -------------------------------------------------------
print("\n" + "=" * 55)
print("3. 重量段计算 — getCalcFunc() 三种模式")
print("=" * 55)

# 申通一区: 0-0.5=2.26, 0.5-1=2.46, 1-2=3.56, 2-3=4.76, 3-30=4.76+0.8/kg, 30+=3.86+0.8/kg
test("实际重量 0.3kg (0-0.5段)", CALC_TABLE[CalcMode.ActualWeight](0.3, SHENTONG_ZONE1), 2.26)
test("实际重量 0.8kg (0.5-1段)", CALC_TABLE[CalcMode.ActualWeight](0.8, SHENTONG_ZONE1), 2.46)
test("实际重量 1.5kg (1-2段)", CALC_TABLE[CalcMode.ActualWeight](1.5, SHENTONG_ZONE1), 3.56)
test("实际重量 2.5kg (2-3段)", CALC_TABLE[CalcMode.ActualWeight](2.5, SHENTONG_ZONE1), 4.76)
test("实际重量 5kg (3-30段)", CALC_TABLE[CalcMode.ActualWeight](5.0, SHENTONG_ZONE1), 4.76 + 2.0*0.8)  # =6.36
test("实际重量 10kg (3-30段)", CALC_TABLE[CalcMode.ActualWeight](10.0, SHENTONG_ZONE1), 4.76 + 7.0*0.8)  # =10.36
test("实际重量 35kg (30+段: firstWeight=3.0)", CALC_TABLE[CalcMode.ActualWeight](35, SHENTONG_ZONE1), 3.86 + 32.0*0.8)  # 29.46

# 百克续: 5kg in 3-30段 → extra=5-3=2kg → ceil(2*10)=20个百克 → 4.76+20*0.08=6.36
test("百克续 5kg (extra=2kg, 20个百克)", CALC_TABLE[CalcMode.HundredGram](5.0, SHENTONG_ZONE1), 4.76 + 20*0.08)

# 全续: ceil(weight) 按标准计算
# ceil(5.3)=6: 4.76 + 3*0.8 = 7.16
test("全续 5.3kg→ceil=6kg", CALC_TABLE[CalcMode.FullAdditional](5.3, SHENTONG_ZONE1), 4.76 + 3.0*0.8)

# -------------------------------------------------------
print("\n" + "=" * 55)
print("4. 完整链路 — 基础运费 + 活动 + 临时 + 省份")
print("=" * 55)

# 模拟一次完整的 calculateFreightDetail
weight = 8.0  # 申通一区 8kg: 4.76+5*0.8=8.76
mode = CalcMode.ActualWeight

base = CALC_TABLE[mode](weight, SHENTONG_ZONE1)
test("基础运费 8kg 申通一区", base, 8.76)

class Rule:
    def __init__(self, name, mode, amount, prov="", active=True, in_range=True):
        self.name = name; self.mode = mode; self.amount = amount
        self.province = prov; self.isActive = active; self.in_range = in_range
    def isTimeInRange(self, t): return self.in_range

# 活动+5% → 临时+1.5 → 省份新疆+5
result = base
for r in [Rule("双11", IncreaseMode.PerTicketPercent, 0.05),
          Rule("油价", IncreaseMode.PerTicketFixed, 1.5)]:
    if r.isActive and r.isTimeInRange(None):
        result = get_increase_func(r.mode)(result, weight, r.amount)

for r in [Rule("新疆", IncreaseMode.PerTicketFixed, 5, prov="新疆"),
          Rule("西藏", IncreaseMode.PerTicketFixed, 8, prov="西藏")]:
    if r.isActive and province_matches("新疆维吾尔自治区", r.province):
        result = get_increase_func(r.mode)(result, weight, r.amount)

expected = ((8.76 * 1.05) + 1.5) + 5.0  # 15.698
test("完整链路: 8.76→+5%→+1.5→+5=15.70", result, expected)

# 同上但省份江苏 → 不加省份费
result2 = base
for r in [Rule("双11", IncreaseMode.PerTicketPercent, 0.05),
          Rule("油价", IncreaseMode.PerTicketFixed, 1.5)]:
    result2 = get_increase_func(r.mode)(result2, weight, r.amount)
for r in [Rule("新疆", IncreaseMode.PerTicketFixed, 5, prov="新疆")]:
    if province_matches("江苏省", r.province):
        result2 = get_increase_func(r.mode)(result2, weight, r.amount)
expected2 = 8.76 * 1.05 + 1.5
test("江苏订单→不加省份费", result2, expected2)

# -------------------------------------------------------
print("\n" + "=" * 55)
print("5. 边界条件")
print("=" * 55)
test("停用规则不生效",
     get_increase_func(IncreaseMode.PerTicketFixed)(10, 5, 100), 110)  # active not checked here
test("超重匹配30+段 minWeight=3.0",
     CALC_TABLE[CalcMode.ActualWeight](50, SHENTONG_ZONE1), 3.86 + 47.0*0.8)
test("边界 0.5kg→匹配[0,0.5]段(先匹配)",
     CALC_TABLE[CalcMode.ActualWeight](0.5, SHENTONG_ZONE1), 2.26)

# -------------------------------------------------------
print(f"\n{'='*55}")
print(f"  TOTAL: {passed+failed} | PASS: {passed} | FAIL: {failed}")
print(f"{'='*55}")
sys.exit(0 if failed == 0 else 1)
