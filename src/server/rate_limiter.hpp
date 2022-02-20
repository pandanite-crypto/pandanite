#include <chrono>
#include <string>
#include <map>

class RateLimiter {
    public:
        RateLimiter(size_t N, size_t seconds);
        bool limit(const std::string &s);
    private:
        const size_t N;
        const size_t seconds;
        std::map<std::string, std::chrono::steady_clock::time_point> c;
}; 