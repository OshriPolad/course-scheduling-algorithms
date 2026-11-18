# Course Scheduling — Algorithms Assignment

**Team members:** _Oshri Polad_, _[Member 2]_, _[Member 3]_

A C++ program that solves three optimization problems on a list of courses:

| Part | Goal | Method |
|------|------|--------|
| COUNT | Max number of non-overlapping courses in one room | Greedy, earliest finish |
| ROOMS | Schedule all courses in the minimum number of rooms | Greedy, by start time, earliest-freed room |
| ENROLLMENTS | Max total enrollments in one room | Dynamic programming (weighted interval scheduling) |

Both sort orders use a hand-written merge sort (no library sort).

## Files

| File | Role |
|------|------|
| `Course.h` | `Course` struct |
| `Algorithms.h` | All function declarations |
| `Algorithms.cpp` | Merge sort (both orders), greedy COUNT, greedy ROOMS, DP (scan + optional binary search), exhaustive checks, helpers. No `main`. |
| `main.cpp` | Reads input, runs the algorithms, prints COUNT / ROOMS / ENROLLMENTS |
| `tests.cpp` | doctest test suite (edge cases, p[j] critique test, random comparison with exhaustive solutions) |
| `doctest.h` | doctest 2.5.0 single-header library (external dependency, MIT license) |
| `testdata/` | `sample`, `empty`, `ties` input/expected-output pairs from the assignment |

## Build and run

Tested with **g++ 13.3.0** (Ubuntu 24.04).

Main program:
```bash
g++ -Wall -Wextra main.cpp Algorithms.cpp -o courses
./courses < testdata/sample.in
```

Compare with the expected output:
```bash
for t in sample empty ties; do ./courses < testdata/$t.in | diff - testdata/$t.out && echo "$t OK"; done
```

Tests:
```bash
g++ -Wall -Wextra tests.cpp Algorithms.cpp -o tests
./tests
```

On Windows (Command Prompt) use `courses.exe < testdata\sample.in` and `tests.exe`.

## Input / output format

Input: `n`, then `n` lines of `id start finish enrollments` (times in minutes from midnight).

```
3
Algorithms 540 600 4
Databases 600 660 4
Operating_Systems 540 660 11
```

Output:
```
COUNT
2 8
Algorithms 540 600 4
Databases 600 660 4
ROOMS
2
1 Algorithms 540 600 4
1 Databases 600 660 4
2 Operating_Systems 540 660 11
ENROLLMENTS
1 11
Operating_Systems 540 660 11
```

## Tests

- Last run: 18 test cases, 87,445 assertions, all passed.
- Random tests use `std::mt19937` with fixed seed `20261118` (values mapped with `%`, so the inputs are identical on every compiler).
  - **Small:** 3000 inputs, n in [0,5], start in [0,30), length in [1,15], enrollments in [1,20]. Each result is compared with the matching exhaustive function.
  - **Large:** 200 inputs, n in [6,300], start in [0,1440), length in [1,180], enrollments in [1,500]. Validity only, plus: rooms = max simultaneous courses, scan and binary-search DP agree.
- A dedicated test (`p[j] must look further back than j-1`) catches the incorrect predecessor rule from the critique section.
