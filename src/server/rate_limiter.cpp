#include "rate_limiter.hpp"


RateLimiter::RateLimiter(size_t N, size_t seconds) : N(N), seconds(seconds){}

bool RateLimiter::limit(const std::string &s) {
    const auto add = std::chrono::seconds(seconds) / N;
    auto iter = c.find(s);
    auto now = std::chrono::steady_clock::now();
    if (iter == c.end()) {
        auto t = now - std::chrono::seconds(seconds) + add;
        c.emplace(std::make_pair(s, t));
        for (auto it = c.cbegin(); it != c.cend();) {
            if (now - it->second > std::chrono::seconds(seconds)) {
                c.erase(it++);
            } else {
                ++it;
            }
        }
        return true;
    } else {
        if (iter->second > now) {
            return false;
        }
        iter->second = std::max(now - std::chrono::seconds(seconds), iter->second) + add;
        return true;
    }
}
