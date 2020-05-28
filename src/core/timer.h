#pragma once
#include <chrono>

namespace Vertex
{
    class Timer final
    {
    public:
        /**
         * @brief Returns current time in seconds.
         * @return Time in seconds.
         */
        static double getTime()
        {
            auto now = std::chrono::system_clock::now();

            return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() / double(SECOND);
        }

    private:
        static const long long SECOND = 1000000000L;
    };
}

