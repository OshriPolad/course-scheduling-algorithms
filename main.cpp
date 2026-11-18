#include <iostream>
#include <vector>
#include "Algorithms.h"

namespace {

void printCourse(const Course& c) {
    std::cout << c.id << ' ' << c.start << ' ' << c.finish << ' ' << c.enrollments << '\n';
}

void printSelection(const char* title, const std::vector<Course>& chosen) {
    std::cout << title << '\n';
    std::cout << chosen.size() << ' ' << totalEnrollments(chosen) << '\n';
    for (const Course& c : chosen) printCourse(c);
}

} // namespace

int main() {
    std::size_t n = 0;
    if (!(std::cin >> n)) n = 0;

    std::vector<Course> original(n);
    for (Course& c : original) {
        std::cin >> c.id >> c.start >> c.finish >> c.enrollments;
    }

    // COUNT and ENROLLMENTS use a copy sorted by finish time.
    std::vector<Course> byFinish = original;
    mergeSort(byFinish);
    std::vector<Course> countResult = selectCourses(byFinish);
    std::vector<Course> enrollResult = selectCoursesDP(byFinish);

    // ROOMS receives the original list and sorts its own copy.
    std::vector<std::vector<Course>> rooms = allocateClassrooms(original);

    printSelection("COUNT", countResult);

    std::cout << "ROOMS\n" << rooms.size() << '\n';
    for (std::size_t r = 0; r < rooms.size(); ++r) {
        for (const Course& c : rooms[r]) {
            std::cout << (r + 1) << ' ';
            printCourse(c);
        }
    }

    printSelection("ENROLLMENTS", enrollResult);
    return 0;
}
