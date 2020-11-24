#pragma once

#include <chrono>

namespace framework
{
    using std::chrono::steady_clock;
    using std::chrono::duration;
    using std::chrono::milliseconds;


    class ScopedTimer
    {
    public:
        /// Starts the timer.
        ScopedTimer();
        ~ScopedTimer() = default;


        /// Stops the timer and returns the time. Only call once until next call to restart().
        long long stopLong();

        /// Stops the timer and returns the time. Only call once until next call to restart().
        milliseconds stop();

        void restart();

    private:
        ScopedTimer& operator= (const ScopedTimer&) = delete;
        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer(const ScopedTimer&&) = delete;

    private:
        steady_clock::time_point m_start;
    };

}