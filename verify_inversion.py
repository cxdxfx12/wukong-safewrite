#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""验证重量倒挂问题"""

def calculate_freight(weight, province):
    """修复后的运费计算"""
    if weight <= 0:
        return 0
    
    region_mapping = {
        '江苏': 1, '浙江': 1, '安徽': 1, '上海': 1,
        '山东': 2, '广东': 2, '福建': 2, '北京': 2,
        '河南': 2, '湖北': 2, '湖南': 2, '江西': 2,
        '天津': 2, '河北': 2,
        '山西': 3, '广西': 3, '四川': 3, '重庆': 3,
        '陕西': 3, '贵州': 3, '辽宁': 3, '吉林': 3,
        '黑龙江': 3, '云南': 3,
        '海南': 4, '甘肃': 4, '青海': 4, '内蒙古': 4,
        '宁夏': 4,
        '新疆': 5, '西藏': 5
    }

    price_rules = {
        1: [(0, 0.5, 2.26, 0), (0.51, 1, 2.46, 0), (1, 2, 3.56, 0), (2, 3, 4.76, 0), (3, 30, 3.76, 0.8), (30, -1, 3.86, 0.8)],
        2: [(0, 0.5, 2.26, 0), (0.51, 1, 2.46, 0), (1, 2, 3.56, 0), (2, 3, 4.76, 0), (3, 30, 3.76, 1.1), (30, -1, 4.06, 1.3)],
        3: [(0, 0.5, 2.26, 0), (0.51, 1, 2.46, 0), (1, 2, 3.56, 0), (2, 3, 4.76, 0), (3, 30, 3.76, 1.5), (30, -1, 4.06, 1.6)],
        4: [(0, 0.5, 2.56, 0), (0.51, 1, 3.56, 0), (1, 2, 4.06, 0), (2, 3, 5.06, 0), (3, 30, 3.76, 2.5), (30, -1, 4.06, 4.3)],
        5: [(0, 0.5, 10, 0), (0.51, 1, 13, 0), (1, 2, 20, 0), (2, 3, 25, 0), (3, 30, 15, 15), (30, -1, 15, 15)]
    }

    normalized = province
    region = region_mapping.get(normalized, 2)
    rules = price_rules[region]

    for seg in rules:
        min_w, max_w, first_price, add_price = seg
        if max_w < 0:
            max_w = float('inf')
        
        if weight >= min_w and weight <= max_w:
            if add_price == 0:
                return first_price
            if max_w == float('inf') and min_w >= 30:
                first_weight = 3.0
            else:
                first_weight = min_w
            extra = weight - first_weight
            return first_price + extra * add_price
    
    return 0


print('=' * 70)
print('重量倒挂验证 - 一区（江苏）')
print('=' * 70)
print()

# 测试 2-4kg 之间的所有 0.1kg 步长
prev_freight = 0
for w in [2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0, 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0]:
    freight = calculate_freight(w, '江苏')
    marker = ''
    if prev_freight > freight:
        marker = f'⚠️ 倒挂！比{w-0.1:.1f}kg贵{prev_freight-freight:.2f}元'
    print(f'{w:4.1f}kg: {freight:6.2f}元 {marker}')
    prev_freight = freight

print()
print('=' * 70)
print('详细计算:')
print('=' * 70)
print(f'2.5kg: 在 2-3kg 段，固定价格 = 4.76 元')
print(f'3.0kg: 在 2-3kg 段，固定价格 = 4.76 元')
print(f'3.1kg: 在 3-30kg 段，首重 3.76 + (3.1-3.0)*0.8 = 3.76 + 0.08 = 3.84 元')
print(f'3.5kg: 在 3-30kg 段，首重 3.76 + (3.5-3.0)*0.8 = 3.76 + 0.40 = 4.16 元')
print(f'4.0kg: 在 3-30kg 段，首重 3.76 + (4.0-3.0)*0.8 = 3.76 + 0.80 = 4.56 元')
print()
print(f'⚠️ 倒挂区间：3.0kg (4.76 元) → 3.1kg (3.84 元)')
print(f'   最大倒挂差额：4.76 - 3.84 = 0.92 元')
print(f'   影响重量范围：3.0kg 到约 3.7kg')
print()

# 计算倒挂结束点
print('倒挂结束点计算:')
print('  3.76 + (w-3.0)*0.8 = 4.76')
print('  (w-3.0)*0.8 = 1.0')
print('  w-3.0 = 1.25')
print('  w = 4.25kg')
print()
print('结论：3.0kg 到 4.25kg 之间的运费都低于 3.0kg 的 4.76 元')
