# Two interview questions

Reconstruction of two problems from an interview round, with optimal solutions
and brute-force cross-checks.

```
python3 interview/test_solutions.py
```

---

## Question 2 — Maximum distance from an occupied seat

*(taken first because it is the one that is pinned down exactly)*

Given a row of seats, `'O'` occupied and `'U'` unoccupied, return the index of
the unoccupied seat whose distance to the nearest occupied seat is largest.

```
['O', 'U', 'U', 'U', 'O', 'O']  ->  index 2 (distance 2)
```

This is **LeetCode 849, "Maximize Distance to Closest Person"**, with the
answer being the seat index rather than the distance itself. Ties resolve to
the smallest index.

**Two-pass, O(n) time / O(n) space** — `best_seat_two_pass`. Sweep left to
right recording the distance to the nearest occupied seat on the left, sweep
right to left folding in the nearest on the right, take the min per seat and
the max over seats. This is the approach described in the round.

**One-pass, O(n) time / O(1) space** — `best_seat_one_pass`. Only three kinds
of empty runs can hold the answer:

| run | best seat | distance |
| --- | --- | --- |
| leading `[0, first)` | `0` | `first` |
| interior gap `(prev, i)` | `prev + (i - prev) // 2` | `(i - prev) // 2` |
| trailing `(last, n)` | `n - 1` | `n - 1 - last` |

so one scan that remembers the previous occupied index suffices.

Edge cases both handle: no unoccupied seat returns `(-1, -1)`; no occupied seat
raises, since every distance would be infinite.

---

## Question 1 — Sweetness distribution

The exact constraints are lost, so `sweetness_distribution.py` covers the four
readings of *"two arrays `A` and `B` of `n` sweetness values, `M` students,
minimize the total sweetness cost"* that fit that description. Each is
implemented optimally and validated against a brute force.

| variant | problem | optimal | brute force |
| --- | --- | --- | --- |
| **A** `min_total_pick_m` | sweet `i` can be made with sweetness `A[i]` or `B[i]`; serve `M` of the `n` sweets | O(n log n) greedy | `C(n, M) · 2^M` |
| **B** `min_total_split` | all `n` sweets are handed out, exactly `M` from batch `A` | O(n log n) greedy | `C(n, M)` |
| **C** `min_total_pairs` | `M` students each get one `A` sweet and one `B` sweet, cost `|a − b|` | O(n²·M) DP | `C(n,M)² · M!` |
| **D** `min_spread` | `M` students, one sweet each from the pooled `2n`, minimize sweetest − least sweet | O(n log n) window | `C(2n, M)` |

**A.** The two recipes for a sweet are independent of every other sweet, so
sweet `i` is worth `min(A[i], B[i])` if used at all; take the `M` cheapest.
Swapping a costlier used sweet for a cheaper unused one never hurts.

**B.** Start from "everything from `B`", costing `sum(B)`. Moving sweet `i` to
batch `A` shifts the total by `d[i] = A[i] − B[i]`, and the moves are
independent, so take the `M` smallest `d[i]`. If an optimum moves `i` but not
`j` with `d[j] < d[i]`, swapping improves it by `d[i] − d[j] > 0` — so the
greedy choice is optimal. This is **LeetCode 1029, "Two City Scheduling"**
with `M` free rather than fixed at `n / 2`.

**C.** The one that actually needs work. Sort both arrays; then an optimal
matching never crosses, because for `a₁ ≤ a₂`, `b₁ ≤ b₂`

```
|a₁ − b₁| + |a₂ − b₂|  ≤  |a₁ − b₂| + |a₂ − b₁|
```

Uncrossing therefore never increases the cost, which turns the problem into a
subsequence alignment over prefixes:

```
dp[i][j][k] = min( dp[i-1][j][k],                          skip A[i-1]
                   dp[i][j-1][k],                          skip B[j-1]
                   dp[i-1][j-1][k-1] + |A[i-1] - B[j-1]| ) pair them
```

O(n²·M) time, O(n·M) space by rolling the `i` dimension.

**D.** Pool and sort all `2n` sweets. The chosen `M` are contiguous in sorted
order — replacing an outlier with a value already inside the window can only
shrink the spread — so slide a window of size `M`. This is the classic
"chocolate distribution" problem.

If the real constraint was something else (a per-student budget, each student
needing a minimum sweetness, or a max−min objective under a fixed total), the
useful pattern is still the one shared by A, B and D: **sort by the quantity the
objective actually moves on** — the per-item cost, the difference `A[i] − B[i]`,
or the pooled value — and let a greedy or window pass do the rest. Only the
pairing objective in C resists that and needs the DP.

## Sources

- [LeetCode 849 — Maximize Distance to Closest Person](https://leetcode.com/problems/maximize-distance-to-closest-person/)
- [LeetCode 1029 — Two City Scheduling](https://algo.monster/liteproblems/1029)
- [Chocolate Distribution Problem — GeeksforGeeks](https://www.geeksforgeeks.org/dsa/chocolate-distribution-problem/)
- [LeetCode 1231 — Divide Chocolate](https://algo.monster/liteproblems/1231)
