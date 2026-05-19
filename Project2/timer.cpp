#include <chrono>

namespace {
    using clock = std::chrono::steady_clock;
    static const auto g_start_time = clock::now();
}

long long GetGameTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        clock::now() - g_start_time).count();
}
