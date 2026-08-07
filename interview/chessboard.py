"""Question 1 — Validate a two-color chessboard.

A board is valid when it contains exactly two distinct colors and every pair of
horizontally or vertically adjacent cells holds opposite colors.

Two equivalent formulations are implemented:

``is_valid_chessboard``
    Parity approach. Pick the top-left color as the reference; every cell whose
    ``(row + col)`` parity is even must match it, every odd cell must differ.
    Time ``O(rows * cols)``, space ``O(1)``.

``is_valid_chessboard_adjacent``
    Direct definition. Compare each cell with its right and bottom neighbour.
    Same complexity, useful in an interview to show the two views agree.
"""

from typing import Hashable, List, Sequence

Board = Sequence[Sequence[Hashable]]


def is_valid_chessboard(board: Board) -> bool:
    """Return True when ``board`` is a valid two-color chessboard.

    Time:  O(rows * cols) — every cell is read once.
    Space: O(1) — only the two color references are held.
    """
    if not board or not board[0]:
        return False

    cols = len(board[0])
    if any(len(row) != cols for row in board):
        return False

    even_color = board[0][0]
    odd_color = None

    for r, row in enumerate(board):
        for c, cell in enumerate(row):
            if (r + c) % 2 == 0:
                if cell != even_color:
                    return False
            else:
                if odd_color is None:
                    if cell == even_color:
                        return False
                    odd_color = cell
                elif cell != odd_color:
                    return False

    # A single-cell board (or any board with no odd-parity cell) never shows a
    # second color, so it cannot satisfy "exactly two colors".
    return odd_color is not None


def is_valid_chessboard_adjacent(board: Board) -> bool:
    """Same check written straight from the adjacency definition.

    Time:  O(rows * cols)
    Space: O(1)
    """
    if not board or not board[0]:
        return False

    rows, cols = len(board), len(board[0])
    if any(len(row) != cols for row in board):
        return False

    colors = set()
    for r in range(rows):
        for c in range(cols):
            colors.add(board[r][c])
            if len(colors) > 2:
                return False
            if c + 1 < cols and board[r][c] == board[r][c + 1]:
                return False
            if r + 1 < rows and board[r][c] == board[r + 1][c]:
                return False

    return len(colors) == 2


def build_chessboard(rows: int, cols: int, a: Hashable = "B", b: Hashable = "W") -> List[List[Hashable]]:
    """Helper: generate a valid board, handy for tests and demos."""
    return [[a if (r + c) % 2 == 0 else b for c in range(cols)] for r in range(rows)]


if __name__ == "__main__":
    examples = [
        [["B", "W"], ["W", "B"]],
        [["B", "W", "B"], ["W", "B", "W"]],
        [["B", "B"], ["W", "B"]],
        [["B", "W"], ["B", "W"]],
        [["B", "B"], ["B", "B"]],
    ]
    for board in examples:
        print(board, "->", is_valid_chessboard(board))
