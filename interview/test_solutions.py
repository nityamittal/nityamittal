"""Randomized checks: every optimal solution is compared against a brute force.

Run with `python3 interview/test_solutions.py` (no test framework required).
"""

import random
from itertools import combinations, permutations, product

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


if __name__ == "__main__":
    random.seed(0)
    check_seats()
    check_sweetness()
    print("all checks passed")
