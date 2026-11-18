#ifndef COURSE_H
#define COURSE_H

#include <string>

// A single course meeting on a given day.
// Times are minutes from the start of the day; start < finish.
struct Course {
    std::string id;          // unique English name, no spaces (e.g. Operating_Systems)
    int start;               // start time in minutes
    int finish;              // finish time in minutes
    long long enrollments;   // positive number of enrolled students (the weight)
};

#endif // COURSE_H
