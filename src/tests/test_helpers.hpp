#include "../core/helpers.hpp"
#include <ctime>
using namespace std;

TEST(time_serialization) {
    std::uint64_t t = getCurrentTime();
    ASSERT_EQUAL(t, stringToTime(timeToString(t)));
}