#pragma once
#include <cstddef>
#include <cstdlib>
#include <iostream>

inline std::size_t testChecks = 0;
inline void testCheck(bool passed, const char* expression, int line) {
    ++testChecks;
    if (!passed) {
        std::cerr << "CHECK failed at line " << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

// Evaluate once, including setup calls, even when NDEBUG is defined.
#define CHECK(expression) testCheck(static_cast<bool>(expression), #expression, __LINE__)
