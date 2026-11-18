#include "Algorithms.h"

#include <algorithm>   // std::reverse, std::max (no library sorting is used)

namespace {

// ---------- Comparators for the two required sort orders ----------

// COUNT / ENROLLMENTS order: finish, then start, then id.
bool lessByFinish(const Course& a, const Course& b) {
    if (a.finish != b.finish) return a.finish < b.finish;
    if (a.start != b.start) return a.start < b.start;
    return a.id < b.id;
}

// ROOMS order: start, then finish, then id.
bool lessByStart(const Course& a, const Course& b) {
    if (a.start != b.start) return a.start < b.start;
    if (a.finish != b.finish) return a.finish < b.finish;
    return a.id < b.id;
}

using Less = bool (*)(const Course&, const Course&);

// Merge the sorted ranges [lo, mid) and [mid, hi) of v, using buffer tmp.
void merge(std::vector<Course>& v, std::vector<Course>& tmp,
           std::size_t lo, std::size_t mid, std::size_t hi, Less less) {
    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        // Take from the right only if strictly smaller -> stable merge.
        if (less(v[j], v[i])) tmp[k++] = v[j++];
        else                  tmp[k++] = v[i++];
    }
    while (i < mid) tmp[k++] = v[i++];
    while (j < hi)  tmp[k++] = v[j++];
    for (std::size_t t = lo; t < hi; ++t) v[t] = tmp[t];
}

// Recursively sort v[lo, hi).
void mergeSortRange(std::vector<Course>& v, std::vector<Course>& tmp,
                    std::size_t lo, std::size_t hi, Less less) {
    if (hi - lo <= 1) return;                 // base case: 0 or 1 element
    std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(v, tmp, lo, mid, less);
    mergeSortRange(v, tmp, mid, hi, less);
    merge(v, tmp, lo, mid, hi, less);
}

void mergeSortWith(std::vector<Course>& courses, Less less) {
    if (courses.size() <= 1) return;
    std::vector<Course> tmp(courses.size());
    mergeSortRange(courses, tmp, 0, courses.size(), less);
}

// Shared DP: fills the table from given predecessors and reconstructs.
// dp[j] = best total using only the first j sorted courses (indices 0..j-1).
//   dp[0] = 0
//   dp[j] = max(dp[j-1], e(j-1) + dp[p[j-1] + 1])
// On a tie between including and excluding, the course is excluded.
std::vector<Course> solveWeightedDP(const std::vector<Course>& sorted,
                                    const std::vector<int>& p) {
    const std::size_t n = sorted.size();
    std::vector<long long> dp(n + 1, 0);
    for (std::size_t j = 1; j <= n; ++j) {
        long long exclude = dp[j - 1];
        long long include = sorted[j - 1].enrollments + dp[p[j - 1] + 1];
        dp[j] = std::max(exclude, include);
    }

    // Reconstruction from the end.
    std::vector<Course> chosen;
    std::size_t j = n;
    while (j > 0) {
        long long include = sorted[j - 1].enrollments + dp[p[j - 1] + 1];
        if (include > dp[j - 1]) {            // strictly better -> include
            chosen.push_back(sorted[j - 1]);
            j = static_cast<std::size_t>(p[j - 1] + 1);
        } else {                              // tie or worse -> exclude
            --j;
        }
    }
    std::reverse(chosen.begin(), chosen.end());   // chronological order
    return chosen;
}

bool noPairOverlaps(const std::vector<Course>& courses, unsigned mask) {
    const std::size_t n = courses.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!(mask & (1u << i))) continue;
        for (std::size_t k = i + 1; k < n; ++k) {
            if ((mask & (1u << k)) && overlaps(courses[i], courses[k])) return false;
        }
    }
    return true;
}

// Assign course idx to a room label in [0, usedRooms] (restricted growth,
// so partitions differing only by room numbers are skipped).
void searchPartitions(const std::vector<Course>& courses, std::size_t idx,
                      std::vector<int>& room, std::size_t usedRooms,
                      std::size_t& best) {
    if (usedRooms >= best) return;            // cannot improve
    if (idx == courses.size()) {
        best = usedRooms;
        return;
    }
    for (std::size_t r = 0; r <= usedRooms; ++r) {
        bool ok = true;
        for (std::size_t k = 0; k < idx && ok; ++k) {
            if (room[k] == static_cast<int>(r) && overlaps(courses[k], courses[idx])) ok = false;
        }
        if (!ok) continue;
        room[idx] = static_cast<int>(r);
        searchPartitions(courses, idx + 1, room,
                         r == usedRooms ? usedRooms + 1 : usedRooms, best);
    }
}

} // namespace

// ---------- Public helpers ----------

bool overlaps(const Course& a, const Course& b) {
    return a.start < b.finish && b.start < a.finish;
}

long long totalEnrollments(const std::vector<Course>& courses) {
    long long sum = 0;
    for (const Course& c : courses) sum += c.enrollments;
    return sum;
}

void mergeSort(std::vector<Course>& courses) {
    mergeSortWith(courses, lessByFinish);
}

void mergeSortByStart(std::vector<Course>& courses) {
    mergeSortWith(courses, lessByStart);
}

// ---------- COUNT ----------

std::vector<Course> selectCourses(const std::vector<Course>& sortedCourses) {
    std::vector<Course> chosen;
    bool any = false;
    int lastFinish = 0;
    for (const Course& c : sortedCourses) {
        if (!any || c.start >= lastFinish) {  // fits after the last chosen course
            chosen.push_back(c);
            lastFinish = c.finish;
            any = true;
        }
    }
    return chosen;
}

// ---------- ROOMS ----------

std::vector<std::vector<Course>> allocateClassrooms(const std::vector<Course>& courses) {
    std::vector<Course> sorted = courses;     // do not modify the input
    mergeSortByStart(sorted);

    std::vector<std::vector<Course>> rooms;
    std::vector<int> lastFinish;              // lastFinish[r] = finish of last course in room r

    for (const Course& c : sorted) {
        int bestRoom = -1;
        for (std::size_t r = 0; r < rooms.size(); ++r) {
            if (lastFinish[r] <= c.start &&
                (bestRoom == -1 || lastFinish[r] < lastFinish[bestRoom])) {
                bestRoom = static_cast<int>(r);   // strict '<' keeps the smallest index on ties
            }
        }
        if (bestRoom == -1) {                 // no free room -> open a new one
            rooms.push_back({});
            lastFinish.push_back(0);
            bestRoom = static_cast<int>(rooms.size()) - 1;
        }
        rooms[bestRoom].push_back(c);
        lastFinish[bestRoom] = c.finish;
    }
    return rooms;
}

// ---------- ENROLLMENTS ----------

std::vector<int> computePredecessorsScan(const std::vector<Course>& sortedCourses) {
    const std::size_t n = sortedCourses.size();
    std::vector<int> p(n, -1);
    for (std::size_t j = 0; j < n; ++j) {
        for (int i = static_cast<int>(j) - 1; i >= 0; --i) {
            if (sortedCourses[i].finish <= sortedCourses[j].start) {
                p[j] = i;
                break;
            }
        }
    }
    return p;
}

std::vector<int> computePredecessorsBinary(const std::vector<Course>& sortedCourses) {
    const std::size_t n = sortedCourses.size();
    std::vector<int> p(n, -1);
    for (std::size_t j = 0; j < n; ++j) {
        // Finish times are non-decreasing; find the last i in [0, j) with finish <= start_j.
        int lo = 0, hi = static_cast<int>(j) - 1, ans = -1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (sortedCourses[mid].finish <= sortedCourses[j].start) {
                ans = mid;
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }
        p[j] = ans;
    }
    return p;
}

std::vector<Course> selectCoursesDP(const std::vector<Course>& sortedCourses) {
    return solveWeightedDP(sortedCourses, computePredecessorsScan(sortedCourses));
}

std::vector<Course> selectCoursesDPBinary(const std::vector<Course>& sortedCourses) {
    return solveWeightedDP(sortedCourses, computePredecessorsBinary(sortedCourses));
}

// ---------- Exhaustive checks (tests only, n <= 5) ----------

std::size_t optimalCountExhaustive(const std::vector<Course>& courses) {
    const std::size_t n = courses.size();
    std::size_t best = 0;
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        if (!noPairOverlaps(courses, mask)) continue;
        std::size_t count = 0;
        for (std::size_t i = 0; i < n; ++i) if (mask & (1u << i)) ++count;
        best = std::max(best, count);
    }
    return best;
}

long long optimalEnrollmentsExhaustive(const std::vector<Course>& courses) {
    const std::size_t n = courses.size();
    long long best = 0;
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        if (!noPairOverlaps(courses, mask)) continue;
        long long sum = 0;
        for (std::size_t i = 0; i < n; ++i) if (mask & (1u << i)) sum += courses[i].enrollments;
        best = std::max(best, sum);
    }
    return best;
}

std::size_t minClassroomsExhaustive(const std::vector<Course>& courses) {
    if (courses.empty()) return 0;
    std::vector<int> room(courses.size(), -1);
    std::size_t best = courses.size();       // one room per course always works
    searchPartitions(courses, 0, room, 0, best);
    return best;
}
