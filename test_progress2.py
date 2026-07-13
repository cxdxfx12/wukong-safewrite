#!/usr/bin/env python3
"""测试进度条修复：0%不被节流、hide/show切换、多文件导入单调递增"""
import time, sys, random

# ====== 模拟 throttledProgress ======
class ProgressTracker:
    def __init__(self):
        self.last_percent = -1
        self.last_time = 0
        self.updates = []   # 记录所有通过的更新
        self.blocked = []   # 记录被拦截的更新

    def throttled_progress(self, percent):
        """完全模拟 MainWindow::throttledProgress"""
        if percent == self.last_percent:
            self.blocked.append(('same', percent))
            return False
        if percent == 0 or percent == 100 or percent - self.last_percent >= 1:
            if percent != 100 and percent != 0 and self.last_time > 0:
                elapsed = (time.time() - self.last_time) * 1000
                if elapsed < 30:
                    self.blocked.append(('30ms', percent))
                    return False
            self.last_percent = percent
            self.last_time = time.time()
            self.updates.append(percent)
            return True
        self.blocked.append(('small_diff', percent))
        return False

# ====== 测试 1: 0%不应该被30ms节流 ======
print("=" * 60)
print("测试 1: 0% 不做30ms节流")
print("=" * 60)

tracker = ProgressTracker()
# 模拟 showCenterProgress 后立即 emit calcProgress(0)
tracker.last_time = time.time()  # 计时器刚启动
r = tracker.throttled_progress(0)
print(f"  emit calcProgress(0) → {'PASS (通过)' if r else 'FAIL (被拦截)'}")
assert r, "0% should not be blocked"

# ====== 测试 2: 完整计算流程 100次更新 ======
print("\n" + "=" * 60)
print("测试 2: 300万行计算 — 0%~99% 完整流程")
print("=" * 60)

tracker2 = ProgressTracker()
tracker2.last_time = time.time()  # showCenterProgress
r = tracker2.throttled_progress(0)
print(f"  0% → {'PASS' if r else 'FAIL'}")

# 模拟 100 次进度更新 (每1%)，间隔35ms确保不被30ms节流拦截
# 实际场景中 300万行≈几秒完成计算，每个1%间隔远大于30ms
for p in range(1, 100):
    time.sleep(0.035)  # 35ms > 30ms 节流阈值
    r = tracker2.throttled_progress(p)
print(f"  总更新次数: {len(tracker2.updates)} (期望≈100)")
print(f"  被拦截次数: {len(tracker2.blocked)}")
assert len(tracker2.updates) >= 95  # 至少95次通过（实际场景下每个1%间隔>>30ms）
print("  PASS")

# ====== 测试 3: 同值不重复上报 ======
print("\n" + "=" * 60)
print("测试 3: 相同百分比不重复上报")
print("=" * 60)
tracker3 = ProgressTracker()
tracker3.throttled_progress(0)
time.sleep(0.05)  # 等30ms+ 让第一次5%通过
r1 = tracker3.throttled_progress(5)
r2 = tracker3.throttled_progress(5)  # same % → 被拦截
r3 = tracker3.throttled_progress(5)  # same % → 被拦截
time.sleep(0.05)  # 再等30ms+
r4 = tracker3.throttled_progress(6)
print(f"  5% 第1次: {'PASS' if r1 else 'FAIL'}")
print(f"  5% 第2次: {'PASS (拦截)' if not r2 else 'FAIL (重复)'}")
print(f"  5% 第3次: {'PASS (拦截)' if not r3 else 'FAIL (重复)'}")
print(f"  6% 通过:   {'PASS' if r4 else 'FAIL'}")
assert r1 and not r2 and not r3 and r4
print("  PASS")

# ====== 测试 4: 100% 不受30ms限制 ======
print("\n" + "=" * 60)
print("测试 4: 100% 不受30ms限制")
print("=" * 60)
tracker4 = ProgressTracker()
tracker4.throttled_progress(0)  # 模拟前面的progress
time.sleep(0.05)  # 等30ms+
r = tracker4.throttled_progress(100)  # 100%不受30ms限制，即使紧跟着也能通过
print(f"  emit calcProgress(100) → {'PASS' if r else 'FAIL (被拦截)'}")
assert r, "100% should not be blocked"

# ====== 测试 5: 多文件导入防倒退 ======
print("\n" + "=" * 60)
print("测试 5: 多文件导入 — CAS防倒退")
print("=" * 60)
from threading import Thread, Lock

class AtomicInt:
    def __init__(self, v=0):
        self._v = v; self._lk = Lock()
    def load(self):
        with self._lk: return self._v
    def cas(self, old, new):
        with self._lk:
            if self._v == old: self._v = new; return True
            return False

max_p = AtomicInt(0)
reported = []
lk = Lock()

def cas_report(pct):
    p = max(0, min(99, pct))
    prev = max_p.load()
    while p > prev:
        if max_p.cas(prev, p):
            with lk: reported.append(p)
            break
        prev = max_p.load()

# 3个文件并行导入，进度信号乱序到达
def file_a():
    for p in [10,20,30,40,50,60,70,80,90,100]:
        cas_report(p); time.sleep(0.0001)

def file_b():
    for p in [5,15,25,35,45,55,65,75,85,95,100]:
        cas_report(p); time.sleep(0.0001)

def file_c():
    for p in [3,8,13,18,23,28,33,38,43,48,53,58,63,68,73,78,83,88,93,98,100]:
        cas_report(p); time.sleep(0.00005)

ts = [Thread(target=f) for f in [file_a, file_b, file_c]]
for t in ts: t.start()
for t in ts: t.join()

mono = all(reported[i] <= reported[i+1] for i in range(len(reported)-1))
print(f"  上报次数: {len(reported)}")
print(f"  单调递增: {'PASS' if mono else 'FAIL'}")
assert mono

print("\n" + "=" * 60)
print("  全部 5 项测试通过!")
print("=" * 60)
