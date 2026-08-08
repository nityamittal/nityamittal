"""Randomized checks: every optimal solution is compared against a brute force.

Run with `python3 interview/test_solutions.py` (no test framework required).
"""

import random
from itertools import combinations, permutations, product

from chessboard_valid import (
    is_valid_chessboard,
    is_valid_chessboard_bitmask,
    pack_rows,
)
from seat_distance import best_seat_one_pass, best_seat_two_pass
from sweetness_distribution import (
    min_spread,
    min_total_pairs,
    min_total_pick_m,
    min_total_split,
)


# --- brute forces -----------------------------------------------------------

def brute_seat(seats):
    """Try every unoccupied seat, scan the whole row for its nearest neighbour."""
    occupied = [i for i, s in enumerate(seats) if s == "O"]
    if not occupied:
        raise ValueError("at least one seat must be occupied")
    best_index, best_dist = -1, -1
    for i, s in enumerate(seats):
        if s == "O":
            continue
        d = min(abs(i - j) for j in occupied)
        if d > best_dist:
            best_index, best_dist = i, d
    return best_index, best_dist


def brute_pick_m(A, B, M):
    best = float("inf")
    for chosen in combinations(range(len(A)), M):
        for picks in product(*[(A[i], B[i]) for i in chosen]):
            best = min(best, sum(picks))
    return best


def brute_split(A, B, M):
    best = float("inf")
    for from_a in combinations(range(len(A)), M):
        s = set(from_a)
        best = min(best, sum(A[i] if i in s else B[i] for i in range(len(A))))
    return best


def brute_pairs(A, B, M):
    best = float("inf")
    for sub_a in combinations(A, M):
        for sub_b in combinations(B, M):
            for perm in permutations(sub_b):
                best = min(best, sum(abs(x - y) for x, y in zip(sub_a, perm)))
    return best


def brute_spread(A, B, M):
    pool = A + B
    return min(max(c) - min(c) for c in combinations(pool, M))


def brute_chessboard(grid):
    """Definition: exactly two colors, and every orthogonal neighbour differs."""
    if not grid or not grid[0]:
        return False
    m, n = len(grid), len(grid[0])
    if len({c for row in grid for c in row}) > 2:
        return False
    for i in range(m):
        for j in range(n):
            if i + 1 < m and grid[i][j] == grid[i + 1][j]:
                return False
            if j + 1 < n and grid[i][j] == grid[i][j + 1]:
                return False
    return True


# --- checks -----------------------------------------------------------------

def check_seats():
    assert best_seat_two_pass(["O", "U", "U", "U", "O", "O"]) == (2, 2)
    assert best_seat_one_pass(["O", "U", "U", "U", "O", "O"]) == (2, 2)
    # leading run, trailing run, all-but-one occupied, no free seat
    assert best_seat_one_pass(["U", "U", "O"]) == (0, 2)
    assert best_seat_one_pass(["O", "U", "U"]) == (2, 2)
    assert best_seat_one_pass(["O", "U", "O"]) == (1, 1)
    assert best_seat_one_pass(["O", "O"]) == (-1, -1)
    assert best_seat_two_pass(["O", "O"]) == (-1, -1)

    for _ in range(3000):
        n = random.randint(1, 12)
        seats = [random.choice("OUU") for _ in range(n)]
        if "O" not in seats:
            seats[random.randrange(n)] = "O"
        expected = brute_seat(seats)
        assert best_seat_two_pass(seats) == expected, (seats, expected)
        assert best_seat_one_pass(seats) == expected, (seats, expected)
    print("seat distance: two-pass and one-pass match brute force")


def check_sweetness():
    for _ in range(300):
        n = random.randint(1, 6)
        M = random.randint(0, n)
        A = [random.randint(0, 20) for _ in range(n)]
        B = [random.randint(0, 20) for _ in range(n)]

        assert min_total_pick_m(A, B, M)[0] == brute_pick_m(A, B, M), ("A", A, B, M)
        assert min_total_split(A, B, M)[0] == brute_split(A, B, M), ("B", A, B, M)
        assert min_total_pairs(A, B, M) == brute_pairs(A, B, M), ("C", A, B, M)

        spread_m = random.randint(1, 2 * n)
        assert min_spread(A, B, spread_m)[0] == brute_spread(A, B, spread_m), (
            "D", A, B, spread_m
        )
    print("sweetness variants A-D: optimal solutions match brute force")


def check_chessboard():
    # the case that kills "just check row 0 and column 0"
    assert is_valid_chessboard([[0, 1], [1, 1]]) is False
    # three colors with all neighbours differing is still not a chessboard
    assert brute_chessboard([[0, 1], [2, 0]]) is False
    assert is_valid_chessboard([[0, 1], [2, 0]]) is False
    assert is_valid_chessboard([[7]]) is True
    assert is_valid_chessboard([]) is False
    assert is_valid_chessboard([[0, 1], [1]]) is False  # ragged

    # exhaustive over every two-color board up to 4x4
    boards = 0
    for m in range(1, 5):
        for n in range(1, 5):
            for bits in product((0, 1), repeat=m * n):
                grid = [list(bits[i * n:(i + 1) * n]) for i in range(m)]
                expected = brute_chessboard(grid)
                assert is_valid_chessboard(grid) == expected, grid
                rows, width = pack_rows(grid, 1)
                assert is_valid_chessboard_bitmask(rows, width) == expected, grid
                boards += 1
    print(f"chessboard: dense and bitmask agree with brute force on {boards} boards")

    # random three-color boards, where the parity shortcut alone is not enough
    for _ in range(2000):
        m, n = random.randint(1, 5), random.randint(1, 5)
        grid = [[random.choice("BWX") for _ in range(n)] for _ in range(m)]
        assert is_valid_chessboard(grid) == brute_chessboard(grid), grid
    print("chessboard: dense check handles boards with a third color")


if __name__ == "__main__":
    random.seed(0)
    check_seats()
    check_sweetness()
    check_chessboard()
    print("all checks passed")
