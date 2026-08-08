"""Question 3: Is an m x n board a valid chessboard?

Valid means the two colors strictly alternate: every cell differs from its
horizontal and vertical neighbours.  Equivalently, cell (i, j) is determined
entirely by the corner and the parity of i + j:

    grid[i][j] == grid[0][0] XOR ((i + j) & 1)

How fast this can be checked depends on how the board arrives:

    representation                       cost
    dense grid of m*n cells              Theta(m * n)  -- optimal, see below
    m rows as n-bit integers             O(m + n)      -- one op per row

The O(m + n) result is the second case: with rows packed into machine words,
each row is validated by a single comparison, so only the first row costs O(n).

Why the dense case cannot beat Theta(m * n): any algorithm that leaves a cell
unread can be defeated by flipping it.  Checking only row 0 and column 0 is
*not* sufficient -- ``[[0, 1], [1, 1]]`` has a perfect first row and first
column and is still not a chessboard.
"""


def is_valid_chessboard(grid):
    """Dense m x n grid of two hashable values (0/1, 'B'/'W', ...).

    Theta(m * n) time, O(1) space -- optimal for this representation.
    An empty board, or one with empty rows, is treated as invalid.
    """
    if not grid or not grid[0]:
        return False
    n = len(grid[0])
    corner = grid[0][0]
    colors = set()

    for i, row in enumerate(grid):
        if len(row) != n:
            return False  # ragged input is not a board
        for j, cell in enumerate(row):
            colors.add(cell)
            expected_matches_corner = (i + j) % 2 == 0
            if (cell == corner) != expected_matches_corner:
                return False

    # A one-cell board is vacuously alternating; anything larger must use
    # exactly two distinct colors, which the parity check above guarantees.
    return len(colors) <= 2


def is_valid_chessboard_bitmask(rows, n):
    """Board given as m integers, row i packed MSB-first into n bits.

    O(m + n): O(n) to verify the first row alternates, then O(1) per row.

    Two conditions are together necessary and sufficient:
      1. row 0 alternates along its length;
      2. every other row equals row 0 (even index) or its complement (odd),
         which is exactly the "column alternates" condition, checked one
         machine word at a time.
    """
    if not rows or n <= 0:
        return False

    full = (1 << n) - 1
    first = rows[0]
    if first & ~full:
        raise ValueError("row wider than n bits")

    # r alternates iff every adjacent bit pair differs, i.e. r ^ (r >> 1)
    # has all of its low n-1 bits set.
    neighbors = (1 << (n - 1)) - 1
    if ((first ^ (first >> 1)) & neighbors) != neighbors:
        return False

    complement = first ^ full
    for i, row in enumerate(rows):
        if row & ~full:
            raise ValueError("row wider than n bits")
        if row != (first if i % 2 == 0 else complement):
            return False
    return True


def pack_rows(grid, one):
    """Helper: dense grid -> (row integers, n), treating `one` as the set bit."""
    n = len(grid[0])
    rows = []
    for row in grid:
        value = 0
        for cell in row:
            value = (value << 1) | (1 if cell == one else 0)
        rows.append(value)
    return rows, n


if __name__ == "__main__":
    board = [[0, 1, 0], [1, 0, 1]]
    print(is_valid_chessboard(board))                       # True
    print(is_valid_chessboard_bitmask(*pack_rows(board, 1)))  # True

    bad = [[0, 1], [1, 1]]  # first row and first column both look fine
    print(is_valid_chessboard(bad))                         # False
    print(is_valid_chessboard_bitmask(*pack_rows(bad, 1)))    # False
