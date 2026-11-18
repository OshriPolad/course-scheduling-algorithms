#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Algorithms.h"

#include <random>
#include <set>
#include <string>
#include <vector>

// ======================================================================
// Helpers for validity checks
// ======================================================================
namespace {

bool sameCourse(const Course& a, const Course& b) {
    return a.id == b.id && a.start == b.start && a.finish == b.finish &&
           a.enrollments == b.enrollments;
}

// Every chosen record appears unchanged in the input.
bool allFromInput(const std::vector<Course>& chosen, const std::vector<Course>& input) {
    for (const Course& c : chosen) {
        bool found = false;
        for (const Course& in : input) if (sameCourse(c, in)) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

bool uniqueIds(const std::vector<Course>& list) {
    std::set<std::string> ids;
    for (const Course& c : list) if (!ids.insert(c.id).second) return false;
    return true;
}

bool noOverlaps(const std::vector<Course>& list) {
    for (std::size_t i = 0; i < list.size(); ++i)
        for (std::size_t j = i + 1; j < list.size(); ++j)
            if (overlaps(list[i], list[j])) return false;
    return true;
}

// Listed in the order the courses take place in the room.
bool chronological(const std::vector<Course>& list) {
    for (std::size_t i = 1; i < list.size(); ++i)
        if (list[i - 1].finish > list[i].start) return false;
    return true;
}

// Full validity of a single-room selection.
void checkSelectionValid(const std::vector<Course>& chosen, const std::vector<Course>& input) {
    CHECK(allFromInput(chosen, input));
    CHECK(uniqueIds(chosen));
    CHECK(noOverlaps(chosen));
    CHECK(chronological(chosen));
}

// Full validity of a room allocation.
void checkAllocationValid(const std::vector<std::vector<Course>>& rooms,
                          const std::vector<Course>& input) {
    std::size_t total = 0;
    std::vector<Course> flat;
    for (const auto& room : rooms) {
        CHECK_FALSE(room.empty());            // no empty room
        CHECK(noOverlaps(room));
        CHECK(chronological(room));
        total += room.size();
        flat.insert(flat.end(), room.begin(), room.end());
    }
    CHECK(total == input.size());             // every course exactly once ...
    CHECK(uniqueIds(flat));                   // ... and no duplicates
    CHECK(allFromInput(flat, input));
}

// Independent lower bound: max number of courses running at the same moment.
std::size_t maxDepth(const std::vector<Course>& courses) {
    std::size_t best = 0;
    for (const Course& t : courses) {
        std::size_t depth = 0;
        for (const Course& c : courses)
            if (c.start <= t.start && t.start < c.finish) ++depth;
        if (depth > best) best = depth;
    }
    return best;
}

std::vector<Course> sortedCopy(std::vector<Course> v) {
    mergeSort(v);
    return v;
}

// Deterministic random input. std::mt19937 output is fixed by the standard;
// we use '%' instead of std::uniform_int_distribution so the sequence is
// identical on every compiler.
std::vector<Course> randomCourses(std::mt19937& rng, std::size_t n,
                                  int maxTime, int maxLen, long long maxEnroll) {
    std::vector<Course> v;
    for (std::size_t i = 0; i < n; ++i) {
        int start = static_cast<int>(rng() % static_cast<unsigned>(maxTime));
        int len = 1 + static_cast<int>(rng() % static_cast<unsigned>(maxLen));
        long long e = 1 + static_cast<long long>(rng() % static_cast<unsigned long long>(maxEnroll));
        v.push_back({"C" + std::to_string(i), start, start + len, e});
    }
    return v;
}

const unsigned SEED = 20261118u;

} // namespace

// ======================================================================
// Examples from the assignment
// ======================================================================

TEST_CASE("sample example from the assignment") {
    std::vector<Course> in = {
        {"Algorithms", 540, 600, 4},
        {"Databases", 600, 660, 4},
        {"Operating_Systems", 540, 660, 11}};
    auto s = sortedCopy(in);

    auto count = selectCourses(s);
    REQUIRE(count.size() == 2);
    CHECK(count[0].id == "Algorithms");
    CHECK(count[1].id == "Databases");
    CHECK(totalEnrollments(count) == 8);

    auto rooms = allocateClassrooms(in);
    REQUIRE(rooms.size() == 2);
    REQUIRE(rooms[0].size() == 2);
    CHECK(rooms[0][0].id == "Algorithms");
    CHECK(rooms[0][1].id == "Databases");
    REQUIRE(rooms[1].size() == 1);
    CHECK(rooms[1][0].id == "Operating_Systems");

    auto dp = selectCoursesDP(s);
    REQUIRE(dp.size() == 1);
    CHECK(dp[0].id == "Operating_Systems");
    CHECK(totalEnrollments(dp) == 11);
}

TEST_CASE("empty input") {
    std::vector<Course> in;
    auto s = sortedCopy(in);
    CHECK(s.empty());
    CHECK(selectCourses(s).empty());
    CHECK(selectCoursesDP(s).empty());
    CHECK(selectCoursesDPBinary(s).empty());
    CHECK(allocateClassrooms(in).empty());
    CHECK(optimalCountExhaustive(in) == 0);
    CHECK(minClassroomsExhaustive(in) == 0);
    CHECK(optimalEnrollmentsExhaustive(in) == 0);
}

TEST_CASE("ties example: equal times and equal enrollments") {
    std::vector<Course> in = {
        {"Databases", 540, 600, 5},
        {"Algorithms", 540, 600, 5},
        {"Operating_Systems", 600, 660, 2}};
    auto s = sortedCopy(in);

    auto count = selectCourses(s);
    REQUIRE(count.size() == 2);
    CHECK(count[0].id == "Algorithms");          // id breaks the tie
    CHECK(count[1].id == "Operating_Systems");

    auto rooms = allocateClassrooms(in);
    REQUIRE(rooms.size() == 2);
    REQUIRE(rooms[0].size() == 2);
    CHECK(rooms[0][0].id == "Algorithms");
    CHECK(rooms[0][1].id == "Operating_Systems"); // tie between free rooms -> room 1
    REQUIRE(rooms[1].size() == 1);
    CHECK(rooms[1][0].id == "Databases");

    auto dp = selectCoursesDP(s);
    REQUIRE(dp.size() == 2);
    CHECK(dp[0].id == "Algorithms");             // tie rule: Databases is excluded
    CHECK(dp[1].id == "Operating_Systems");
    CHECK(totalEnrollments(dp) == 7);
}

// ======================================================================
// Merge sort
// ======================================================================

TEST_CASE("mergeSort: finish, then start, then id; records unchanged") {
    // Catches: sorting by start, missing tie-breakers, lost/duplicated records.
    std::vector<Course> in = {
        {"E", 0, 50, 1}, {"B", 10, 30, 2}, {"A", 10, 30, 3},
        {"C", 5, 30, 4}, {"D", 20, 25, 5}};
    auto s = sortedCopy(in);
    std::vector<std::string> expected = {"D", "C", "A", "B", "E"};
    REQUIRE(s.size() == expected.size());
    for (std::size_t i = 0; i < s.size(); ++i) CHECK(s[i].id == expected[i]);
    CHECK(allFromInput(s, in));
    CHECK(uniqueIds(s));
}

TEST_CASE("mergeSortByStart: start, then finish, then id") {
    std::vector<Course> in = {
        {"E", 0, 50, 1}, {"B", 10, 30, 2}, {"A", 10, 30, 3},
        {"C", 5, 30, 4}, {"D", 10, 25, 5}};
    mergeSortByStart(in);
    std::vector<std::string> expected = {"E", "C", "D", "A", "B"};
    REQUIRE(in.size() == expected.size());
    for (std::size_t i = 0; i < in.size(); ++i) CHECK(in[i].id == expected[i]);
}

TEST_CASE("mergeSort: random inputs give sorted permutation") {
    std::mt19937 rng(SEED);
    for (int t = 0; t < 200; ++t) {
        auto in = randomCourses(rng, rng() % 60, 100, 30, 50);
        auto s = sortedCopy(in);
        REQUIRE(s.size() == in.size());
        CHECK(allFromInput(s, in));
        CHECK(uniqueIds(s));
        for (std::size_t i = 1; i < s.size(); ++i) {
            const Course& a = s[i - 1];
            const Course& b = s[i];
            bool ordered = a.finish < b.finish ||
                           (a.finish == b.finish && (a.start < b.start ||
                           (a.start == b.start && a.id < b.id)));
            CHECK(ordered);
        }
    }
}

// ======================================================================
// Edge cases
// ======================================================================

TEST_CASE("single course") {
    std::vector<Course> in = {{"Only", 100, 200, 7}};
    auto s = sortedCopy(in);
    CHECK(selectCourses(s).size() == 1);
    CHECK(allocateClassrooms(in).size() == 1);
    auto dp = selectCoursesDP(s);
    REQUIRE(dp.size() == 1);
    CHECK(dp[0].enrollments == 7);
}

TEST_CASE("touching endpoints are not an overlap") {
    // Catches: using '>' instead of '>=' (or '<' instead of '<=').
    std::vector<Course> in = {{"A", 0, 10, 1}, {"B", 10, 20, 1}, {"C", 20, 30, 1}};
    auto s = sortedCopy(in);
    CHECK(selectCourses(s).size() == 3);
    CHECK(allocateClassrooms(in).size() == 1);
    CHECK(selectCoursesDP(s).size() == 3);
    CHECK(computePredecessorsScan(s) == std::vector<int>{-1, 0, 1});
}

TEST_CASE("all courses overlap") {
    std::vector<Course> in = {{"A", 0, 100, 3}, {"B", 10, 90, 9}, {"C", 20, 80, 5}, {"D", 50, 60, 1}};
    auto s = sortedCopy(in);
    CHECK(selectCourses(s).size() == 1);
    CHECK(allocateClassrooms(in).size() == 4);
    auto dp = selectCoursesDP(s);
    REQUIRE(dp.size() == 1);
    CHECK(dp[0].id == "B");
}

TEST_CASE("identical times with different ids") {
    std::vector<Course> in = {{"Z", 0, 10, 1}, {"M", 0, 10, 1}, {"A", 0, 10, 1}};
    auto s = sortedCopy(in);
    auto count = selectCourses(s);
    REQUIRE(count.size() == 1);
    CHECK(count[0].id == "A");
    auto rooms = allocateClassrooms(in);
    REQUIRE(rooms.size() == 3);
    CHECK(rooms[0][0].id == "A");
    CHECK(rooms[1][0].id == "M");
    CHECK(rooms[2][0].id == "Z");
}

TEST_CASE("unsorted input: functions do not modify their input") {
    std::vector<Course> in = {{"C", 30, 40, 2}, {"A", 0, 10, 2}, {"B", 5, 35, 9}};
    const std::vector<Course> copy = in;
    auto rooms = allocateClassrooms(in);
    REQUIRE(in.size() == copy.size());
    for (std::size_t i = 0; i < in.size(); ++i) CHECK(sameCourse(in[i], copy[i]));

    auto s = sortedCopy(in);
    const std::vector<Course> sCopy = s;
    selectCourses(s);
    selectCoursesDP(s);
    for (std::size_t i = 0; i < s.size(); ++i) CHECK(sameCourse(s[i], sCopy[i]));
    checkAllocationValid(rooms, in);
}

TEST_CASE("gap between max count and max enrollments") {
    // Catches: solving ENROLLMENTS with the COUNT greedy rule.
    std::vector<Course> in = {{"A", 0, 10, 1}, {"B", 10, 20, 1}, {"Big", 0, 20, 100}};
    auto s = sortedCopy(in);
    CHECK(selectCourses(s).size() == 2);
    auto dp = selectCoursesDP(s);
    REQUIRE(dp.size() == 1);
    CHECK(dp[0].id == "Big");
    CHECK(totalEnrollments(dp) == 100);
}

TEST_CASE("DP tie rule: equal totals -> exclude the current course") {
    // Sorted: A(0,10,5), B(10,20,5), C(0,21,10). Both {A,B} and {C} give 10.
    // At C, include = exclude = 10 -> C is excluded -> {A,B}.
    std::vector<Course> in = {{"C", 0, 21, 10}, {"A", 0, 10, 5}, {"B", 10, 20, 5}};
    auto s = sortedCopy(in);
    auto dp = selectCoursesDP(s);
    REQUIRE(dp.size() == 2);
    CHECK(dp[0].id == "A");
    CHECK(dp[1].id == "B");
    CHECK(totalEnrollments(dp) == 10);
}

TEST_CASE("ROOMS: reuse a freed room and pick the free room that finished earliest") {
    // By start: B(0,10)->room1, A(0,20)->room2, E(5,25)->room3 (none free at 5),
    // C(10,30)->room1 (reuse). D(30,40): rooms free at 30, 20, 25 -> room2.
    std::vector<Course> in = {{"D", 30, 40, 1}, {"A", 0, 20, 1}, {"C", 10, 30, 1},
                              {"B", 0, 10, 1}, {"E", 5, 25, 1}};
    auto rooms = allocateClassrooms(in);
    REQUIRE(rooms.size() == 3);
    REQUIRE(rooms[0].size() == 2);
    CHECK(rooms[0][0].id == "B");
    CHECK(rooms[0][1].id == "C");
    REQUIRE(rooms[1].size() == 2);
    CHECK(rooms[1][0].id == "A");
    CHECK(rooms[1][1].id == "D");
    REQUIRE(rooms[2].size() == 1);
    CHECK(rooms[2][0].id == "E");
    CHECK(minClassroomsExhaustive(in) == 3);
}

// ======================================================================
// Algorithmic critique: the wrong p[j] rule (only checks j-1)
// ======================================================================

TEST_CASE("p[j] must look further back than j-1") {
    // Sorted by finish: A(0,10,5), B(15,40,1), C(20,50,10).
    // Wrong rule: C overlaps B (j-1) -> p[C] = -1.  Correct: p[C] = 0 (A).
    // Wrong rule gives max 10 (C alone); correct answer is A + C = 15.
    std::vector<Course> in = {{"C", 20, 50, 10}, {"A", 0, 10, 5}, {"B", 15, 40, 1}};
    auto s = sortedCopy(in);
    REQUIRE(s[0].id == "A");
    REQUIRE(s[1].id == "B");
    REQUIRE(s[2].id == "C");

    CHECK(computePredecessorsScan(s) == std::vector<int>{-1, 0, 0});

    auto dp = selectCoursesDP(s);
    CHECK(totalEnrollments(dp) == 15);
    REQUIRE(dp.size() == 2);
    CHECK(dp[0].id == "A");
    CHECK(dp[1].id == "C");
    CHECK(optimalEnrollmentsExhaustive(in) == 15);
}

// ======================================================================
// Exhaustive functions on known answers
// ======================================================================

TEST_CASE("exhaustive functions on hand-checked inputs") {
    std::vector<Course> in = {{"A", 0, 10, 4}, {"B", 10, 20, 4}, {"C", 0, 20, 11}};
    CHECK(optimalCountExhaustive(in) == 2);
    CHECK(minClassroomsExhaustive(in) == 2);
    CHECK(optimalEnrollmentsExhaustive(in) == 11);

    std::vector<Course> five = {{"A", 0, 5, 1}, {"B", 1, 6, 1}, {"C", 2, 7, 1},
                                {"D", 3, 8, 1}, {"E", 4, 9, 1}};
    CHECK(optimalCountExhaustive(five) == 1);
    CHECK(minClassroomsExhaustive(five) == 5);
    CHECK(optimalEnrollmentsExhaustive(five) == 1);
}

// ======================================================================
// Random comparison with exhaustive solutions (n <= 5)
// ======================================================================

TEST_CASE("random small inputs match exhaustive solutions") {
    // 3000 inputs, n in [0,5], start in [0,30), length in [1,15], enrollments in [1,20].
    // Small time range -> many overlaps, touching endpoints and equal times.
    std::mt19937 rng(SEED);
    for (int t = 0; t < 3000; ++t) {
        std::size_t n = rng() % 6;
        auto in = randomCourses(rng, n, 30, 15, 20);
        auto s = sortedCopy(in);

        auto count = selectCourses(s);
        auto rooms = allocateClassrooms(in);
        auto dp = selectCoursesDP(s);
        auto dpBin = selectCoursesDPBinary(s);

        checkSelectionValid(count, in);
        checkSelectionValid(dp, in);
        checkAllocationValid(rooms, in);

        CHECK(count.size() == optimalCountExhaustive(in));
        CHECK(rooms.size() == minClassroomsExhaustive(in));
        CHECK(totalEnrollments(dp) == optimalEnrollmentsExhaustive(in));

        // Optional extension: same predecessors and same list.
        CHECK(computePredecessorsScan(s) == computePredecessorsBinary(s));
        REQUIRE(dp.size() == dpBin.size());
        for (std::size_t i = 0; i < dp.size(); ++i) CHECK(sameCourse(dp[i], dpBin[i]));
    }
}

// ======================================================================
// Random large inputs: validity only (no exhaustive)
// ======================================================================

TEST_CASE("random large inputs: valid output and consistent results") {
    // 200 inputs, n in [6,300], start in [0,1440), length in [1,180], enrollments in [1,500].
    std::mt19937 rng(SEED + 1);
    for (int t = 0; t < 200; ++t) {
        std::size_t n = 6 + rng() % 295;
        auto in = randomCourses(rng, n, 1440, 180, 500);
        auto s = sortedCopy(in);

        auto count = selectCourses(s);
        auto rooms = allocateClassrooms(in);
        auto dp = selectCoursesDP(s);
        auto dpBin = selectCoursesDPBinary(s);

        checkSelectionValid(count, in);
        checkSelectionValid(dp, in);
        checkAllocationValid(rooms, in);

        // Rooms equals the independent lower bound (max simultaneous courses).
        CHECK(rooms.size() == maxDepth(in));
        // COUNT has at least as many courses as any valid set (e.g. the DP set),
        // and DP has at least the enrollments of any valid set (e.g. the COUNT set).
        CHECK(count.size() >= dp.size());
        CHECK(totalEnrollments(dp) >= totalEnrollments(count));

        CHECK(computePredecessorsScan(s) == computePredecessorsBinary(s));
        CHECK(totalEnrollments(dp) == totalEnrollments(dpBin));
    }
}
