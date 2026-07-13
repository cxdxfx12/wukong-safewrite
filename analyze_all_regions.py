#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析所有区域的价格倒挂问题"""

def check_region_inversion(region_name, rules):
    """检查某个区域是否存在价格倒挂"""
    print(f'\n{region_name}')
    print('-' * 60)
    
    inversions = []
    prev_w = 0
    prev_freight = 0
    
    # 从 0.1kg 到 50kg，步长 0.1kg
    for w_int in range(1, 500):
        w = w_int / 10.0
        
        # 计算运费
        freight = 0
        for seg in rules:
            min_w, max_w, first_price, add_price = seg
            if max_w < 0:
                max_w = float('inf')
            
            if w >= min_w and w <= max_w:
                if add_price == 0:
                    freight = first_price
                else:
                    if max_w == float('inf') and min_w >= 30:
                        first_weight = 3.0
                    else:
                        first_weight = min_w
                    extra = w - first_weight
                    freight = first_price + extra * add_price
                break
        
        # 检查倒挂
        if prev_freight > freight and (prev_freight - freight) > 0.01:
            inversions.append({
                'from_w': prev_w,
                'from_freight': prev_freight,
                'to_w': w,
                'to_freight': freight,
                'diff': prev_freight - freight
            })
        
        prev_w = w
        prev_freight = freight
    
    # 报告倒挂
    if inversions:
        print(f'  发现 {len(inversions)} 处价格倒挂:')
        for inv in inversions[:5]:  # 只显示前 5 个
            print(f'    {inv["from_w"]:.1f}kg ({inv["from_freight"]:.2f}元) -> {inv["to_w"]:.1f}kg ({inv["to_freight"]:.2f}元), 差额：{inv["diff"]:.2f}元')
        if len(inversions) > 5:
            print(f'    ... 还有 {len(inversions)-5} 处')
    else:
        print('  无价格倒挂')
    
    return inversions


# 各区域价格规则
regions = {
    '一区（江苏/浙江/安徽/上海）': [(0, 0.5, 2.26, 0), (0.51, 1, 2.46, 0), (1, 2, 3.56, 0), (2, 3, 4.76, 0), (3, 30, 3.76, 0.8), (30, -1, 3.86, 0.8)],
    '二区（山东/广东/福建等）': [(0, 0.5, 2.26, 0), (0.51, 1, 2.46, 0), (1, 2, 3.56, 0), (2, 3, 4.76, 0), (3, 30, 3.76, 1.1), (30, -1, 4.06, 1.3)],
    '三区（山西/广西/四川等）': [(0, 0.5, 2.26, 0), (0.51, 1, 2.46, 0), (1, 2, 3.56, 0), (2, 3, 4.76, 0), (3, 30, 3.76, 1.5), (30, -1, 4.06, 1.6)],
    '四区（海南/甘肃/青海等）': [(0, 0.5, 2.56, 0), (0.51, 1, 3.56, 0), (1, 2, 4.06, 0), (2, 3, 5.06, 0), (3, 30, 3.76, 2.5), (30, -1, 4.06, 4.3)],
    '五区（新疆）': [(0, 0.5, 10, 0), (0.51, 1, 13, 0), (1, 2, 20, 0), (2, 3, 25, 0), (3, 30, 15, 15), (30, -1, 15, 15)],
    '五区（西藏）': [(0, 0.5, 13, 0), (0.51, 1, 15, 0), (1, 2, 25, 0), (2, 3, 30, 0), (3, 30, 15, 15), (30, -1, 15, 15)],
}

print('=' * 70)
print('全国各区域价格倒挂分析')
print('=' * 70)

all_inversions = {}
for name, rules in regions.items():
    inversions = check_region_inversion(name, rules)
    all_inversions[name] = inversions

print('\n' + '=' * 70)
print('总结')
print('=' * 70)

for name, inversions in all_inversions.items():
    if inversions:
        max_diff = max(inv['diff'] for inv in inversions)
        print(f'{name}: {len(inversions)} 处倒挂，最大差额 {max_diff:.2f}元')
    else:
        print(f'{name}: 无倒挂')
