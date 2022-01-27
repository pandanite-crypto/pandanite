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
// You can use it in your callbacks: if (ratelimiter.check_in( key)){proceed}else{fail} 
// where as key you can take the IP which you can get from your uWebsocket endpoint callbacks:
// void callback(uWS::HttpResponse<false> *res,  uWS::HttpRequest *) {
//     auto key=res->getRemoteAddress();
// }