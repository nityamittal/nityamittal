"""Tests for both interview questions. Run: python3 interview/test_solutions.py"""

import random
import unittest

from chessboard import (
    build_chessboard,
    is_valid_chessboard,
    is_valid_chessboard_adjacent,
)
from lcs import (
    lcs_memoized,
    lcs_recursive,
    lcs_space_optimized,
    lcs_tabulation,
    lcs_tabulation_with_string,
)


class TestChessboard(unittest.TestCase):
    IMPLS = (is_valid_chessboard, is_valid_chessboard_adjacent)

    def check(self, board, expected):
        for impl in self.IMPLS:
            with self.subTest(impl=impl.__name__, board=board):
                self.assertEqual(impl(board), expected)

    def test_valid_boards(self):
        self.check([["B", "W"], ["W", "B"]], True)
        self.check([["W", "B"], ["B", "W"]], True)
        self.check([["B", "W", "B"], ["W", "B", "W"]], True)
        self.check([[0, 1], [1, 0]], True)
        self.check([["B", "W"]], True)
        self.check([["B"], ["W"]], True)

    def test_invalid_boards(self):
        self.check([["B", "B"], ["W", "B"]], False)  # horizontal clash
        self.check([["B", "W"], ["B", "W"]], False)  # vertical clash
        self.check([["B", "B"], ["B", "B"]], False)  # one color
        self.check([["B", "W", "W"], ["W", "B", "B"]], False)

    def test_degenerate_input(self):
        self.check([], False)
        self.check([[]], False)
        self.check([["B"]], False)  # only one color present
        self.check([["B", "W"], ["W"]], False)  # ragged rows

    def test_more_than_two_colors(self):
        self.check([["B", "W"], ["W", "G"]], False)
        self.check([["B", "W", "B"], ["W", "G", "W"]], False)

    def test_generated_boards_are_valid(self):
        for rows in range(1, 8):
            for cols in range(1, 8):
                board = build_chessboard(rows, cols)
                expected = rows * cols > 1  # 1x1 shows only one color
                self.check(board, expected)

    def test_single_flip_breaks_validity(self):
        rng = random.Random(7)
        for _ in range(50):
            rows, cols = rng.randint(2, 6), rng.randint(2, 6)
            board = build_chessboard(rows, cols)
            r, c = rng.randrange(rows), rng.randrange(cols)
            board[r][c] = "W" if board[r][c] == "B" else "B"
            self.check(board, False)


class TestLCS(unittest.TestCase):
    IMPLS = (lcs_recursive, lcs_memoized, lcs_tabulation, lcs_space_optimized)

    def check(self, a, b, expected):
        for impl in self.IMPLS:
            with self.subTest(impl=impl.__name__, a=a, b=b):
                self.assertEqual(impl(a, b), expected)
        with self.subTest(impl="lcs_tabulation_with_string"):
            self.assertEqual(lcs_tabulation_with_string(a, b)[0], expected)

    def test_known_cases(self):
        self.check("abcde", "ace", 3)
        self.check("abc", "abc", 3)
        self.check("abc", "def", 0)
        self.check("AGGTAB", "GXTXAYB", 4)
        self.check("bl", "yby", 1)

    def test_empty_strings(self):
        self.check("", "", 0)
        self.check("abc", "", 0)
        self.check("", "abc", 0)

    def test_symmetry_and_length_bound(self):
        self.check("bsbininm", "jmjkbkjkv", 1)
        self.check("jmjkbkjkv", "bsbininm", 1)
        self.check("aaaa", "aa", 2)
        self.check("aa", "aaaa", 2)

    def test_reconstructed_subsequence_is_common(self):
        pairs = [("abcde", "ace"), ("AGGTAB", "GXTXAYB"), ("abc", "def"), ("", "x")]
        for a, b in pairs:
            length, seq = lcs_tabulation_with_string(a, b)
            with self.subTest(a=a, b=b):
                self.assertEqual(len(seq), length)
                self.assertTrue(is_subsequence(seq, a))
                self.assertTrue(is_subsequence(seq, b))

    def test_all_implementations_agree_on_random_input(self):
        rng = random.Random(42)
        for _ in range(200):
            a = "".join(rng.choice("abc") for _ in range(rng.randint(0, 8)))
            b = "".join(rng.choice("abc") for _ in range(rng.randint(0, 8)))
            expected = lcs_recursive(a, b)
            self.check(a, b, expected)

    def test_larger_inputs_skip_brute_force(self):
        rng = random.Random(11)
        for _ in range(20):
            a = "".join(rng.choice("abcd") for _ in range(120))
            b = "".join(rng.choice("abcd") for _ in range(90))
            expected = lcs_tabulation(a, b)
            self.assertEqual(lcs_memoized(a, b), expected)
            self.assertEqual(lcs_space_optimized(a, b), expected)
            length, seq = lcs_tabulation_with_string(a, b)
            self.assertEqual(length, expected)
            self.assertTrue(is_subsequence(seq, a) and is_subsequence(seq, b))


def is_subsequence(needle: str, haystack: str) -> bool:
    it = iter(haystack)
    return all(ch in it for ch in needle)


if __name__ == "__main__":
    unittest.main(verbosity=2)
