"""Question 2 — Longest Common Subsequence, four ways.

The interview walked through the standard progression, each step trading work
for memory or memory for time:

===========================  ===================  ==================
Approach                     Time                 Space
===========================  ===================  ==================
plain recursion              O(2^(n+m))           O(n + m) stack
recursion + memoization      O(n * m)             O(n * m) + stack
bottom-up tabulation         O(n * m)             O(n * m)
space-optimized tabulation   O(n * m)             O(min(n, m))
===========================  ===================  ==================

Recurrence, for prefixes ``a[:i]`` and ``b[:j]``::

    lcs(0, j) = lcs(i, 0) = 0
    lcs(i, j) = 1 + lcs(i - 1, j - 1)                 if a[i-1] == b[j-1]
    lcs(i, j) = max(lcs(i - 1, j), lcs(i, j - 1))     otherwise

``lcs_tabulation_with_string`` also reconstructs one actual subsequence by
walking the table backwards, which is the usual follow-up once the length is
on the board.
"""

from functools import lru_cache


def lcs_recursive(a: str, b: str) -> int:
    """Plain recursion, no memo — the brute-force baseline.

    Time:  O(2^(n + m)) — each mismatch branches twice.
    Space: O(n + m) recursion stack.
    """

    def solve(i: int, j: int) -> int:
        if i == 0 or j == 0:
            return 0
        if a[i - 1] == b[j - 1]:
            return 1 + solve(i - 1, j - 1)
        return max(solve(i - 1, j), solve(i, j - 1))

    return solve(len(a), len(b))


def lcs_memoized(a: str, b: str) -> int:
    """Top-down recursion with memoization.

    Each of the (n + 1) * (m + 1) states is solved once.

    Time:  O(n * m)
    Space: O(n * m) memo table + O(n + m) stack.
    """

    @lru_cache(maxsize=None)
    def solve(i: int, j: int) -> int:
        if i == 0 or j == 0:
            return 0
        if a[i - 1] == b[j - 1]:
            return 1 + solve(i - 1, j - 1)
        return max(solve(i - 1, j), solve(i, j - 1))

    try:
        return solve(len(a), len(b))
    finally:
        solve.cache_clear()


def lcs_tabulation(a: str, b: str) -> int:
    """Bottom-up tabulation — the version the interviewer asked for.

    ``dp[i][j]`` is the LCS length of ``a[:i]`` and ``b[:j]``. Row 0 and
    column 0 stay zero (an empty prefix shares nothing), so the loops start
    at 1 and every read of ``dp[i-1][...]`` / ``dp[...][j-1]`` is already
    filled in.

    Time:  O(n * m)
    Space: O(n * m)
    """
    n, m = len(a), len(b)
    dp = [[0] * (m + 1) for _ in range(n + 1)]

    for i in range(1, n + 1):
        for j in range(1, m + 1):
            if a[i - 1] == b[j - 1]:
                dp[i][j] = 1 + dp[i - 1][j - 1]
            else:
                dp[i][j] = max(dp[i - 1][j], dp[i][j - 1])

    return dp[n][m]


def lcs_space_optimized(a: str, b: str) -> int:
    """Tabulation keeping only two rows, over the shorter string.

    Row ``i`` depends on row ``i - 1`` alone, so the full table is never
    needed. Iterating columns over the shorter string bounds the rows by
    ``min(n, m)``.

    Time:  O(n * m)
    Space: O(min(n, m))
    """
    if len(b) > len(a):
        a, b = b, a  # keep the row length (and therefore the space) minimal

    m = len(b)
    prev = [0] * (m + 1)
    curr = [0] * (m + 1)

    for i in range(1, len(a) + 1):
        for j in range(1, m + 1):
            if a[i - 1] == b[j - 1]:
                curr[j] = 1 + prev[j - 1]
            else:
                curr[j] = max(prev[j], curr[j - 1])
        prev, curr = curr, prev

    return prev[m]


def lcs_tabulation_with_string(a: str, b: str) -> tuple[int, str]:
    """Length plus one longest common subsequence, reconstructed from the table.

    Backtracking from ``dp[n][m]``: on a character match both indices step
    back and the character joins the answer; otherwise follow the larger
    neighbour. Ties are broken arbitrarily — any one valid LCS is returned.

    Time:  O(n * m) to fill, O(n + m) to walk back.
    Space: O(n * m)
    """
    n, m = len(a), len(b)
    dp = [[0] * (m + 1) for _ in range(n + 1)]

    for i in range(1, n + 1):
        for j in range(1, m + 1):
            if a[i - 1] == b[j - 1]:
                dp[i][j] = 1 + dp[i - 1][j - 1]
            else:
                dp[i][j] = max(dp[i - 1][j], dp[i][j - 1])

    out = []
    i, j = n, m
    while i > 0 and j > 0:
        if a[i - 1] == b[j - 1]:
            out.append(a[i - 1])
            i -= 1
            j -= 1
        elif dp[i - 1][j] >= dp[i][j - 1]:
            i -= 1
        else:
            j -= 1

    return dp[n][m], "".join(reversed(out))


if __name__ == "__main__":
    pairs = [("abcde", "ace"), ("abc", "abc"), ("abc", "def"), ("AGGTAB", "GXTXAYB")]
    for a, b in pairs:
        length, seq = lcs_tabulation_with_string(a, b)
        print(f"{a!r} vs {b!r} -> {length} ({seq!r})")
