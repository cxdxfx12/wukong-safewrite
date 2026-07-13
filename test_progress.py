#!/usr/bin/env python3
"""
模拟多文件导入 + 大批量计算的进度条行为
"""
import random, math, sys
from threading import Thread, Lock
from time import sleep
from collections import defaultdict

# ====== 模拟 C++ QAtomicInt CAS ======
class AtomicInt:
    def __init__(self, v=0):
        self._v = v
        self._lock = Lock()
    def load(self):
        with self._lock: return self._v
    def fetch_add(self, n):
        with self._lock:
            old = self._v
            self._v += n
            return old
    def cas(self, old, new):
        with self._lock:
            if self._v == old:
                self._v = new
                return True
            return False

# ====== 模拟多文件导入进度 ======
def test_import_progress():
    print("=" * 60)
    print("测试 1: 多文件导入进度 — 单调递增检查")
    print("=" * 60)

    # 3个文件，分别有 50万、120万、80万行
    files = [
        {"name": "申通1月.xlsx", "rows": 500000},
        {"name": "申通2月.xlsx", "rows": 1200000},
        {"name": "申通3月.xlsx", "rows": 800000},
    ]
    total = sum(f["rows"] for f in files)

    processed = AtomicInt(0)
    max_progress = AtomicInt(0)
    reported = []  # 记录所有上报的进度值
    lock = Lock()

    def cas_report(raw_percent):
        p = max(0, min(99, raw_percent))
        prev = max_progress.load()
        while p > prev:
            if max_progress.cas(prev, p):
                with lock:
                    reported.append(p)
                break
            prev = max_progress.load()

    def import_file(file_info):
        rows = file_info["rows"]
        # 每个文件随机分成 20 个内部进度点
        milestones = sorted(random.sample(range(1, rows), min(19, rows-1))) + [rows]
        prev_milestone = 0
        for m in milestones:
            chunk = m - prev_milestone
            # 模拟导入延迟
            sleep(random.uniform(0.001, 0.003))
            # 文件内部进度百分比
            file_pct = int(m * 100.0 / rows)
            local_done = m  # 本文件已读行数
            global_done = processed.load() + local_done
            raw_pct = int(global_done * 100.0 / total)
            cas_report(raw_pct)
            prev_milestone = m
        # 文件导入完成
        processed.fetch_add(rows)

    # 并行跑
    threads = [Thread(target=import_file, args=(f,)) for f in files]
    for t in threads: t.start()
    for t in threads: t.join()

    # 验证：进度单调递增
    ok = all(reported[i] <= reported[i+1] for i in range(len(reported)-1))
    status = "PASS" if ok else "FAIL"
    print(f"  总行数: {total:,}")
    print(f"  上报次数: {len(reported)}")
    print(f"  单调递增: {status}")
    if not ok:
        for i in range(len(reported)-1):
            if reported[i] > reported[i+1]:
                print(f"    BACKWARD at {i}: {reported[i]} -> {reported[i+1]}")
    return ok


# ====== 模拟 300万行计算进度 ======
def test_calc_progress():
    print("\n" + "=" * 60)
    print("测试 2: 300万行计算进度 — 每1%递增，最多100次更新")
    print("=" * 60)

    total = 3_000_000
    threads_count = 8
    chunk_size = 5000  # 每块5000行
    total_chunks = total // chunk_size

    processed = AtomicInt(0)
    last_percent = AtomicInt(-1)
    reported = []
    lock = Lock()

    def report_progress(percent):
        prev = last_percent.load()
        while percent > prev:
            if last_percent.cas(prev, percent):
                with lock:
                    reported.append(percent)
                    done = int(percent / 100.0 * total)
                    if percent < 100:
                        print(f"\r  正在计算运费... {done:>10,} / {total:,} 条  [{percent:>3}%]", end="")
                break
            prev = last_percent.load()

    def calc_chunks(chunks):
        for _ in range(chunks):
            done = processed.fetch_add(chunk_size) + chunk_size
            percent = int(done * 100.0 / total)
            report_progress(percent)
            sleep(random.uniform(0.0001, 0.0005))

    # 分配chunks给各线程
    chunks_per_thread = total_chunks // threads_count
    remainder = total_chunks % threads_count

    threads = []
    for i in range(threads_count):
        n = chunks_per_thread + (1 if i < remainder else 0)
        if n > 0:
            threads.append(Thread(target=calc_chunks, args=(n,)))

    for t in threads: t.start()
    for t in threads: t.join()

    print(f"\n  总行数: {total:,}")
    print(f"  线程数: {threads_count}")
    print(f"  上报次数: {len(reported)} (理想≈100)")
    ok = len(reported) <= 101 and all(reported[i] <= reported[i+1] for i in range(len(reported)-1))
    status = "PASS" if ok else "FAIL"
    print(f"  结果: {status}")
    return ok


# ====== 测试 3: 小文件（5000行）计算进度 ======
def test_small_calc():
    print("\n" + "=" * 60)
    print("测试 3: 小文件(5000行)计算进度 — 按1%也能更新约50次")
    print("=" * 60)

    total = 5000
    processed = AtomicInt(0)
    last_percent = AtomicInt(-1)
    reported = []
    lock = Lock()

    def report_progress(percent):
        prev = last_percent.load()
        while percent > prev:
            if last_percent.cas(prev, percent):
                with lock: reported.append(percent)
                break
            prev = last_percent.load()

    # 每100行一块
    for chunk in range(0, total, 100):
        actual = min(100, total - chunk)
        done = processed.fetch_add(actual) + actual
        percent = int(done * 100.0 / total)
        report_progress(percent)

    print(f"  总行数: {total:,}")
    print(f"  上报次数: {len(reported)}")
    # 5000行，每% = 50行，每块100行 = 2%变化
    # 约 50 次更新
    ok = 20 <= len(reported) <= 101
    status = "PASS" if ok else "FAIL"
    print(f"  结果: {status}")
    return ok


# ====== 运行全部 ======
if __name__ == "__main__":
    random.seed(42)
    results = []
    results.append(test_import_progress())
    results.append(test_calc_progress())
    results.append(test_small_calc())

    print("\n" + "=" * 60)
    passed = sum(results)
    total = len(results)
    print(f"  总计: {total} | 通过: {passed} | 失败: {total - passed}")
    print("=" * 60)
    sys.exit(0 if all(results) else 1)
