# tests/ssm/test_ssm_deterministic_model.py
# Bit-Exact Host-Side Verification of Phase 7 SSM Deterministic Scoring Engine

import unittest

SSM_FIXED_SHIFT = 16
SSM_FIXED_ONE   = 1 << SSM_FIXED_SHIFT
INT32_MAX       = 0x7FFFFFFF
INT32_MIN       = -0x80000000

def to_fixed(val: int) -> int:
    return val << SSM_FIXED_SHIFT

def fixed_add_sat(a: int, b: int) -> int:
    res = a + b
    if res > INT32_MAX:
        return INT32_MAX
    if res < INT32_MIN:
        return INT32_MIN
    return res

def fixed_sub_sat(a: int, b: int) -> int:
    res = a - b
    if res > INT32_MAX:
        return INT32_MAX
    if res < INT32_MIN:
        return INT32_MIN
    return res

def fixed_mul_sat(a: int, b: int) -> int:
    res = (a * b) >> SSM_FIXED_SHIFT
    if res > INT32_MAX:
        return INT32_MAX
    if res < INT32_MIN:
        return INT32_MIN
    return res

class TaskMock:
    def __init__(self, tid, name, base_prio, ready_tick, last_cpu, assigned_cpu, affinity_mask, quantum):
        self.id = tid
        self.name = name
        self.base_priority = base_prio
        self.ready_since_tick = ready_tick
        self.last_cpu = last_cpu
        self.assigned_cpu = assigned_cpu
        self.affinity_mask = affinity_mask
        self.quantum = quantum

def ssm_score_candidate(task: TaskMock, current_cpu: int, now_tick: int) -> int:
    # 1. Eligibility Check (Hard Precondition)
    if task is None:
        return INT32_MIN
    if not (task.affinity_mask & (1 << current_cpu)):
        return INT32_MIN
    if task.assigned_cpu != -1 and task.assigned_cpu != current_cpu:
        return INT32_MIN

    # 2. Priority Component (Base priority scaled to Q16.16)
    s_prio = to_fixed(task.base_priority)

    # 3. Aging Component (1 boost per 250 ticks, capped at 31 - base_priority)
    waited = max(0, now_tick - task.ready_since_tick)
    boost = waited // 250
    max_boost = max(0, 31 - task.base_priority)
    boost = min(boost, max_boost)
    s_aging = to_fixed(boost)

    # 4. Locality & Migration Penalty
    s_locality = 0
    if task.last_cpu == current_cpu:
        s_locality = to_fixed(2) # +2.0 cache warm bonus
    elif task.last_cpu != -1:
        s_locality = -fixed_mul_sat(to_fixed(1), int(1.5 * SSM_FIXED_ONE)) # -1.5 migration penalty

    # 5. Quantum Incentive
    q_clamped = min(max(1, task.quantum), 20)
    s_quantum = (q_clamped * (SSM_FIXED_ONE // 2)) // 20 # 0.0 to 0.5

    # Total Score computation
    score = fixed_add_sat(s_prio, s_aging)
    score = fixed_add_sat(score, s_locality)
    score = fixed_add_sat(score, s_quantum)

    return score

class TestSSMDeterministicModel(unittest.TestCase):
    def test_fixed_point_basic_arithmetic(self):
        self.assertEqual(to_fixed(1), 65536)
        self.assertEqual(fixed_add_sat(to_fixed(2), to_fixed(3)), to_fixed(5))
        self.assertEqual(fixed_sub_sat(to_fixed(5), to_fixed(2)), to_fixed(3))
        self.assertEqual(fixed_mul_sat(to_fixed(4), to_fixed(3)), to_fixed(12))

    def test_saturation_limits(self):
        self.assertEqual(fixed_add_sat(INT32_MAX - 10, 20), INT32_MAX)
        self.assertEqual(fixed_sub_sat(INT32_MIN + 10, 20), INT32_MIN)

    def test_eligibility_enforcement(self):
        # Task with affinity pinned to CPU 0 evaluated on CPU 1 -> MUST BE REJECTED
        t = TaskMock(1, "PinnedT", 16, 0, 0, 0, 0x01, 10)
        score = ssm_score_candidate(t, current_cpu=1, now_tick=100)
        self.assertEqual(score, INT32_MIN)

    def test_cache_locality_bonus(self):
        # Task that previously ran on CPU 0 should score higher on CPU 0 than on CPU 1
        t = TaskMock(2, "WorkerT", 16, 100, last_cpu=0, assigned_cpu=-1, affinity_mask=0xFF, quantum=10)
        score_cpu0 = ssm_score_candidate(t, current_cpu=0, now_tick=200)
        score_cpu1 = ssm_score_candidate(t, current_cpu=1, now_tick=200)
        self.assertGreater(score_cpu0, score_cpu1)

    def test_starvation_aging_progression(self):
        # As wait time increases, aging boost increases up to priority ceiling
        t = TaskMock(3, "AgingT", 10, ready_tick=0, last_cpu=-1, assigned_cpu=-1, affinity_mask=0xFF, quantum=10)
        s_initial = ssm_score_candidate(t, current_cpu=0, now_tick=100)
        s_after_500 = ssm_score_candidate(t, current_cpu=0, now_tick=500)
        s_after_2000 = ssm_score_candidate(t, current_cpu=0, now_tick=2000)
        self.assertGreater(s_after_500, s_initial)
        self.assertGreater(s_after_2000, s_after_500)

    def test_determinism_invariant(self):
        # Exact same inputs must produce exact same score across 10,000 iterations
        t = TaskMock(4, "InvariantT", 24, 1500, 2, -1, 0xFF, 15)
        base_score = ssm_score_candidate(t, current_cpu=2, now_tick=2500)
        for _ in range(10000):
            self.assertEqual(ssm_score_candidate(t, current_cpu=2, now_tick=2500), base_score)

if __name__ == "__main__":
    unittest.main()
