#include "sha256_utils.hpp"
#include <sstream>
#include <iomanip>
#include <functional>
#include <openssl/sha.h>

// Function to convert a SHA256Hash to a string
std::string SHA256toString(const SHA256Hash& hash) {
    std::stringstream ss;
    for (const auto& byte : hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

// Function to concatenate two SHA256Hash instances and hash the result
SHA256Hash concatHashes(const SHA256Hash& a, const SHA256Hash& b) {
    SHA256Hash result;
    std::array<uint8_t, a.size() + b.size()> concatenated_hashes;
    std::copy(a.begin(), a.end(), concatenated_hashes.begin());
    std::copy(b.begin(), b.end(), concatenated_hashes.begin() + a.size());
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, concatenated_hashes.data(), concatenated_hashes.size());
    SHA256_Final(result.data(), &sha256);
    
    return result;
}

namespace std {
    // Hash function for SHA256Hash to be used in std::unordered_map
    std::size_t hash<SHA256Hash>::operator()(const SHA256Hash& k) const {
        std::size_t seed = 0;
        for (const auto& byte : k) {
            seed ^= std::hash<uint8_t>{}(byte) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
} // namespace std
