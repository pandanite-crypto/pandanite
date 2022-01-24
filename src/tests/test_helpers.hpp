#include "../core/helpers.hpp"
#include <ctime>
using namespace std;

TEST(time_serialization) {
    std::uint64_t t = getCurrentTime();
    ASSERT_EQUAL(t, stringToUint64(uint64ToString(t)));
}