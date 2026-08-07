# Interview questions

Two problems from a ~45-minute coding round, with each approach the
interviewer asked to see and its complexity.

## 1. Validate a two-color chessboard — `chessboard.py`

Given a 2D array of two colors, decide whether every horizontally or
vertically adjacent pair holds opposite colors.

| Function | Idea | Time | Space |
| --- | --- | --- | --- |
| `is_valid_chessboard` | cell must match the color implied by `(row + col)` parity | `O(rows × cols)` | `O(1)` |
| `is_valid_chessboard_adjacent` | compare each cell with its right/bottom neighbour | `O(rows × cols)` | `O(1)` |

Edge cases both handle: empty input, ragged rows, more than two colors, and a
board showing only one color (e.g. `1×1`), which is rejected since "exactly
two colors" cannot hold.

## 2. Longest Common Subsequence — `lcs.py`

Recurrence over prefixes `a[:i]`, `b[:j]`:

```
lcs(i, j) = 0                                   if i == 0 or j == 0
          = 1 + lcs(i-1, j-1)                   if a[i-1] == b[j-1]
          = max(lcs(i-1, j), lcs(i, j-1))       otherwise
```

| Function | Approach | Time | Space |
| --- | --- | --- | --- |
| `lcs_recursive` | plain recursion | `O(2^(n+m))` | `O(n + m)` stack |
| `lcs_memoized` | top-down + memo | `O(n × m)` | `O(n × m)` |
| `lcs_tabulation` | bottom-up table | `O(n × m)` | `O(n × m)` |
| `lcs_space_optimized` | two rows over the shorter string | `O(n × m)` | `O(min(n, m))` |
| `lcs_tabulation_with_string` | table + backtrack to recover a subsequence | `O(n × m)` | `O(n × m)` |

## Running

```bash
python3 interview/test_solutions.py   # unit tests
python3 interview/chessboard.py       # demo
python3 interview/lcs.py              # demo
```

Tests cross-check every LCS implementation against the brute-force version on
random inputs, and verify each reconstructed subsequence really is a
subsequence of both strings.
