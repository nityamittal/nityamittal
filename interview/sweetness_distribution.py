"""Question 1: Sweetness distribution (reconstructed).

The exact constraints from the interview are lost, so this file collects the
four formulations that "two arrays A and B of n sweetness values, M students,
minimize the total sweetness cost" can reasonably mean.  Each one gets the
optimal algorithm plus the argument for why it is optimal; every brute force
counterpart lives in test_solutions.py.

Summary
-------
    variant                                        optimal            brute force
    A  pick M sweets, each has two recipes         O(n log n) greedy  C(n, M) * 2^M
    B  hand out all n, exactly M come from A       O(n log n) greedy  C(n, M)
    C  M students each get one A and one B sweet   O(n^2 * M) DP      C(n,M)^2 * M!
    D  M students, minimize the sweetness spread   O(n log n) window  C(2n, M)
"""


# ---------------------------------------------------------------------------
# Variant A: each sweet can be prepared two ways; serve M of the n sweets.
# ---------------------------------------------------------------------------

def min_total_pick_m(A, B, M):
    """Sweet i can be made with sweetness A[i] or B[i].  Serve M students, one
    distinct sweet each.  Minimize the total sweetness handed out.

    The two choices are independent per sweet, so sweet i is worth exactly
    min(A[i], B[i]) if it is used at all, and the selection is then just "take
    the M cheapest".  Any solution using a costlier sweet in place of an unused
    cheaper one can be improved by swapping, so the greedy choice is optimal.

    O(n log n); O(n) with quickselect.  Returns (total, chosen_indices).
    """
    _validate(A, B, M, len(A))
    costs = sorted(range(len(A)), key=lambda i: min(A[i], B[i]))
    chosen = sorted(costs[:M])
    return sum(min(A[i], B[i]) for i in chosen), chosen


# ---------------------------------------------------------------------------
# Variant B: every sweet is handed out, exactly M of them from batch A.
# ---------------------------------------------------------------------------

def min_total_split(A, B, M):
    """All n sweets are distributed.  Sweet i costs A[i] if taken from batch A
    and B[i] if taken from batch B, and exactly M must come from batch A.
    Minimize the total.

    Start from "everything comes from B", which costs sum(B).  Moving sweet i
    to batch A changes the total by d[i] = A[i] - B[i], and the moves are
    independent, so taking the M smallest d[i] (negative ones first) is
    optimal.  Exchange argument: if an optimal solution moves i but not j with
    d[j] < d[i], swapping them lowers the total by d[i] - d[j] > 0.

    This is LeetCode 1029 ("Two City Scheduling") with M free instead of n / 2.

    O(n log n).  Returns (total, indices_taken_from_A).
    """
    _validate(A, B, M, len(A))
    order = sorted(range(len(A)), key=lambda i: A[i] - B[i])
    from_a = sorted(order[:M])
    total = sum(B) + sum(A[i] - B[i] for i in from_a)
    return total, from_a


# ---------------------------------------------------------------------------
# Variant C: M students, each served one sweet from A and one from B.
# ---------------------------------------------------------------------------

def min_total_pairs(A, B, M):
    """Form M (a, b) pairs using distinct elements of A and of B.  A student
    served a and b costs |a - b| (the mismatch in sweetness).  Minimize the sum.

    Two facts make this tractable:

      1. In an optimal solution the pairs never cross once A and B are sorted:
         if a1 <= a2 and the solution pairs (a1, b2), (a2, b1) with b1 <= b2,
         then uncrossing to (a1, b1), (a2, b2) never increases
         |a1-b1| + |a2-b2| <= |a1-b2| + |a2-b1|.
      2. So after sorting both arrays the problem is a subsequence alignment,
         and a DP over prefixes covers every non-crossing matching:

             dp[i][j][k] = min cost using A[:i], B[:j] with k pairs formed
                         = min(dp[i-1][j][k],              # skip A[i-1]
                               dp[i][j-1][k],              # skip B[j-1]
                               dp[i-1][j-1][k-1] + |A[i-1] - B[j-1]|)

    O(n^2 * M) time, O(n * M) space by rolling the i dimension.
    Returns the minimum total cost.
    """
    n = len(A)
    _validate(A, B, M, n)
    a, b = sorted(A), sorted(B)
    INF = float("inf")

    # prev[j][k] / cur[j][k] are the dp rows for i-1 and i.
    prev = [[INF] * (M + 1) for _ in range(n + 1)]
    for j in range(n + 1):
        prev[j][0] = 0

    for i in range(1, n + 1):
        cur = [[INF] * (M + 1) for _ in range(n + 1)]
        cur[0][0] = 0
        for j in range(1, n + 1):
            cur[j][0] = 0
            for k in range(1, min(M, i, j) + 1):
                best = prev[j][k]           # skip a[i-1]
                if cur[j - 1][k] < best:    # skip b[j-1]
                    best = cur[j - 1][k]
                if prev[j - 1][k - 1] < INF:
                    paired = prev[j - 1][k - 1] + abs(a[i - 1] - b[j - 1])
                    if paired < best:
                        best = paired
                cur[j][k] = best
        prev = cur

    return prev[n][M]


# ---------------------------------------------------------------------------
# Variant D: M students, minimize the spread of what they receive.
# ---------------------------------------------------------------------------

def min_spread(A, B, M):
    """Pool both batches (2n sweets), give one sweet to each of M students, and
    minimize the difference between the sweetest and the least sweet handed out.

    Sort the pool: the M chosen sweets should be contiguous in sorted order,
    because replacing an outlier with a value already inside the window can
    only shrink the spread.  So slide a window of size M and keep the best.

    This is the classic "chocolate distribution" problem over A + B.
    O(n log n).  Returns (spread, chosen_values).
    """
    _validate(A, B, M, len(A) + len(B))
    pool = sorted(A + B)
    best, start = float("inf"), 0
    for i in range(len(pool) - M + 1):
        spread = pool[i + M - 1] - pool[i]
        if spread < best:
            best, start = spread, i
    return best, pool[start:start + M]


def _validate(A, B, M, available):
    if len(A) != len(B):
        raise ValueError("A and B must have the same length")
    if not 0 <= M <= available:
        raise ValueError(f"M must be between 0 and {available}, got {M}")


if __name__ == "__main__":
    A = [4, 1, 9, 7]
    B = [2, 8, 3, 5]
    print(min_total_pick_m(A, B, 2))  # cheapest two of min(A, B)
    print(min_total_split(A, B, 2))   # exactly two from A
    print(min_total_pairs(A, B, 2))   # two best-matched (a, b) pairs
    print(min_spread(A, B, 3))        # tightest window of three
