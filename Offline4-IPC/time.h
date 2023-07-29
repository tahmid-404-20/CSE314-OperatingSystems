#include <iostream>
#include <chrono>

auto start_time = std::chrono::high_resolution_clock::now();

long long get_time() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    long long elapsed_time_ms = duration.count();
    return elapsed_time_ms;
}