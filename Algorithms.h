#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <string>
#include <vector>
#include <cstddef>
#include "Course.h"

// ---------- Required signatures ----------

// Merge sort by (finish, start, id) ascending. Only reorders the records.
void mergeSort(std::vector<Course>& courses);

// COUNT: greedy by earliest finish. Input must be sorted by mergeSort.
std::vector<Course> selectCourses(
    const std::vector<Course>& sortedCourses);

// ENROLLMENTS: dynamic programming, predecessors found by backward scan, O(n^2).
// Input must be sorted by mergeSort.
std::vector<Course> selectCoursesDP(
    const std::vector<Course>& sortedCourses);

// Brute force (tests only, at most 5 courses, any order).
std::size_t optimalCountExhaustive(
    const std::vector<Course>& courses);

// ROOMS: greedy by earliest start. Input in any order; it is not modified.
std::vector<std::vector<Course>> allocateClassrooms(
    const std::vector<Course>& courses);

// Brute force over all partitions (tests only, at most 5 courses).
std::size_t minClassroomsExhaustive(
    const std::vector<Course>& courses);

// Brute force over all subsets (tests only, at most 5 courses).
long long optimalEnrollmentsExhaustive(
    const std::vector<Course>& courses);

// ---------- Optional extension ----------

// Same as selectCoursesDP, but predecessors are found by binary search,
// O(n log n) including the sort.
std::vector<Course> selectCoursesDPBinary(
    const std::vector<Course>& sortedCourses);

// ---------- Helpers (exposed for testing) ----------

// Merge sort by (start, finish, id) ascending. Used by allocateClassrooms.
void mergeSortByStart(std::vector<Course>& courses);

// p[j] = largest index i < j with sorted[i].finish <= sorted[j].start, or -1.
// Input must be sorted by finish (mergeSort order).
std::vector<int> computePredecessorsScan(const std::vector<Course>& sortedCourses);
std::vector<int> computePredecessorsBinary(const std::vector<Course>& sortedCourses);

// Two courses overlap if they share a positive-length time interval.
// A course starting exactly when another finishes does NOT overlap it.
bool overlaps(const Course& a, const Course& b);

// Sum of enrollments in a list.
long long totalEnrollments(const std::vector<Course>& courses);

#endif // ALGORITHMS_H
