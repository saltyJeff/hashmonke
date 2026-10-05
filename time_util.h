#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>

/**
 * Returns the performance counter frequency in counts per second.
 * Cached on first call.
 */
static inline uint64_t hashmonke_perf_frequency(void)
{
    static uint64_t freq = 0;
    if (freq == 0)
    {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        freq = (uint64_t)li.QuadPart;
    }
    return freq;
}

/**
 * Reads the current performance counter value (raw ticks).
 */
static inline uint64_t hashmonke_perf_counter(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64_t)counter.QuadPart;
}

/**
 * Returns monotonic time in milliseconds.
 */
static inline uint64_t hashmonke_monotonic_ms(void)
{
    return (hashmonke_perf_counter() * 1000) / hashmonke_perf_frequency();
}

/**
 * Returns monotonic time in seconds with fractional precision.
 */
static inline double hashmonke_monotonic_seconds(void)
{
    static double inv_freq = 0.0;
    if (inv_freq == 0.0)
    {
        inv_freq = 1.0 / (double)hashmonke_perf_frequency();
    }
    return (double)hashmonke_perf_counter() * inv_freq;
}

/**
 * Suspends thread execution for the specified milliseconds.
 */
static inline void hashmonke_sleep_ms(unsigned int ms)
{
    Sleep(ms);
}
