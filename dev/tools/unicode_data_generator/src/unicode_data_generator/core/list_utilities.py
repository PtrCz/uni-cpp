from heapq import heappush, heappop


def find_sublist(list: list, sublist: list) -> int:
    # from: https://stackoverflow.com/a/17870684

    sublist_length = len(sublist)

    if sublist_length == 0:
        return 0

    for ind in (i for i,e in enumerate(list) if e==sublist[0]):
        if list[ind:ind + sublist_length]==sublist:
            return ind
    
    return -1


def index_or_append(l: list, value) -> int:
    try:
        return l.index(value)
    
    except ValueError:
        l.append(value)
        return len(l) - 1


def overlap(a: list, b: list) -> int:
    sep = object()
    s = b + [sep] + a

    pi = [0] * len(s)

    for i in range(1, len(s)):
        j = pi[i - 1]

        while j > 0 and s[i] != s[j]:
            j = pi[j - 1]

        if s[i] == s[j]:
            j += 1

        pi[i] = j

    return min(pi[-1], len(b))


def merge_arrays(arr1: list, arr2: list) -> list:
    overlap_ab = overlap(arr1, arr2)
    overlap_ba = overlap(arr2, arr1)

    if overlap_ab >= overlap_ba:
        return arr1 + arr2[overlap_ab:]
    else:
        return arr2 + arr1[overlap_ba:]


def contains_sublist(list: list, sublist: list) -> bool:
    if len(sublist) > len(list):
        return False

    if not sublist:
        return True

    first = sublist[0]
    limit = len(list) - len(sublist) + 1

    for i in range(limit):
        if list[i] == first and list[i:i + len(sublist)] == sublist:
            return True

    return False


def remove_contained(arrays: list[list]) -> list[list]:
    keep = []

    for i, a in enumerate(arrays):
        contained = False

        for j, b in enumerate(arrays):
            if i == j:
                continue

            if len(a) <= len(b) and contains_sublist(b, a):
                contained = True
                break

        if not contained:
            keep.append(a)

    return keep


def shortest_superarray(arrays: list[list]) -> list:
    if not arrays:
        return []

    arrays = remove_contained(arrays)

    next_id = len(arrays)

    active = {i: arr for i, arr in enumerate(arrays)}

    heap = []

    def overlap_score(a, b):
        return max(
            overlap(a, b),
            overlap(b, a),
        )

    ids = list(active.keys())

    for i in range(len(ids)):
        for j in range(i + 1, len(ids)):
            ia = ids[i]
            ib = ids[j]

            score = overlap_score(
                active[ia],
                active[ib],
            )

            heappush(heap, (-score, ia, ib))

    while len(active) > 1:

        while True:
            neg_score, i, j = heappop(heap)

            if i in active and j in active:
                break

        merged = merge_arrays(
            active[i],
            active[j],
        )

        del active[i]
        del active[j]

        new_id = next_id
        next_id += 1

        active[new_id] = merged

        for other_id, other_arr in active.items():
            if other_id == new_id:
                continue

            score = overlap_score(
                merged,
                other_arr,
            )

            heappush(
                heap,
                (-score, new_id, other_id),
            )

    return next(iter(active.values()))


def fast_superarray(arrays: list[list]) -> list:
    superarray = []

    for array in arrays:
        found_in_superarray = False

        for ind in (i for i,e in enumerate(superarray) if e==array[0]):
            if superarray[ind:ind + len(array)]==array:
                found_in_superarray = True
                break

        if not found_in_superarray:
            for value in array:
                superarray.append(value)

    return superarray