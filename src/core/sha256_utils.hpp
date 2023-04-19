#ifndef SHA256_UTILS_HPP
#define SHA256_UTILS_HPP

#include <array>
#include <string>
#include <functional>

// Assuming SHA256Hash is a std::array of 32 bytes (256 bits)
using SHA256Hash = std::array<uint8_t, 32>;
const SHA256Hash NULL_SHA256_HASH = {0};

// Function to convert a SHA256Hash to a string
std::string SHA256toString(const SHA256Hash& hash);

// Function to concatenate two SHA256Hash instances and hash the result
SHA256Hash concatHashes(const SHA256Hash& a, const SHA256Hash& b);

namespace std {
    // Hash function for SHA256Hash to be used in std::unordered_map
    template <>
    struct hash<SHA256Hash> {
        std::size_t operator()(const SHA256Hash& k) const;
    };
} // namespace std

#endif // SHA256_UTILS_HPP
