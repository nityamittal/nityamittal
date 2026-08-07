"""Question 2: Maximum distance from an occupied seat.

Given a row of seats where 'O' is occupied and 'U' is unoccupied, pick the
unoccupied seat whose distance to the nearest occupied seat is as large as
possible, and return that seat's index.

    ['O', 'U', 'U', 'U', 'O', 'O']  ->  index 2  (distance 2)

This is LeetCode 849 ("Maximize Distance to Closest Person") with the answer
being the seat rather than the distance.  Ties go to the smallest index.
"""

OCCUPIED = "O"
UNOCCUPIED = "U"


def best_seat_two_pass(seats):
    """Two passes, O(n) time and O(n) extra space.

    Pass 1 (left to right) records the distance to the nearest occupied seat
    on the left; pass 2 (right to left) folds in the nearest one on the right.
    The answer for a seat is the min of the two, and we take the max of those.

    Returns (index, distance), or (-1, -1) if there is no unoccupied seat.
    Raises ValueError if nobody is seated (every distance is infinite).
    """
    n = len(seats)
    if n == 0 or OCCUPIED not in seats:
        raise ValueError("at least one seat must be occupied")

    INF = float("inf")
    dist = [INF] * n

    nearest = None  # distance to the closest occupied seat on this side
    for i in range(n):
        if seats[i] == OCCUPIED:
            nearest = 0
        elif nearest is not None:
            nearest += 1
        dist[i] = INF if nearest is None else nearest

    nearest = None
    for i in range(n - 1, -1, -1):
        if seats[i] == OCCUPIED:
            nearest = 0
        elif nearest is not None:
            nearest += 1
        if nearest is not None and nearest < dist[i]:
            dist[i] = nearest

    best_index, best_dist = -1, -1
    for i in range(n):
        if seats[i] != OCCUPIED and dist[i] > best_dist:
            best_index, best_dist = i, dist[i]
    return best_index, best_dist


def best_seat_one_pass(seats):
    """One pass over the seats, O(n) time and O(1) extra space.

    Only three kinds of runs of empty seats can hold the answer:

      * a leading run  [0, first)      -> sit at 0, distance = first
      * a trailing run (last, n)       -> sit at n - 1, distance = n - 1 - last
      * an interior gap (prev, i)      -> sit in the middle, distance = (i - prev) // 2

    so we scan once, keeping the index of the previous occupied seat.

    Returns (index, distance), or (-1, -1) if there is no unoccupied seat.
    Raises ValueError if nobody is seated.
    """
    n = len(seats)
    best_index, best_dist = -1, -1
    prev = -1  # index of the last occupied seat seen so far

    for i in range(n):
        if seats[i] != OCCUPIED:
            continue
        if prev == -1:
            if i > 0 and i > best_dist:  # leading run of empty seats
                best_index, best_dist = 0, i
        else:
            gap = (i - prev) // 2
            if gap > 0 and gap > best_dist:  # interior gap
                best_index, best_dist = prev + gap, gap
        prev = i

    if prev == -1:
        raise ValueError("at least one seat must be occupied")

    if prev < n - 1:  # trailing run of empty seats
        tail = n - 1 - prev
        if tail > best_dist:
            best_index, best_dist = n - 1, tail

    return best_index, best_dist


if __name__ == "__main__":
    row = ["O", "U", "U", "U", "O", "O"]
    print(best_seat_two_pass(row))  # (2, 2)
    print(best_seat_one_pass(row))  # (2, 2)
