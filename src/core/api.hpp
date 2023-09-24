#pragma once
#include <functional>
#include "transaction.hpp"
#include "block.hpp"
#include "common.hpp"
#include <optional>

[[nodiscard]] std::optional<Bigint> getTotalWork(string host_url);
[[nodiscard]] std::optional<uint32_t> getCurrentBlockCount(string host_url);
[[nodiscard]] std::optional<json> getName(string host_url);
[[nodiscard]] std::optional<json> getBlockData(string host_url, int idx);
[[nodiscard]] std::optional<json> getMiningProblem(string host_url);
std::string sendTransaction(string host_url, const Transaction& t);
// std::optional<json> sendTransactions(string host_url, vector<Transaction>& transactionList);
[[nodiscard]] std::optional<json> verifyTransaction(string host_url, Transaction& t);
[[nodiscard]] std::optional<json> pingPeer(string host_url, string peer_url, uint64_t time, string version, string networkName);
[[nodiscard]] std::optional<json> submitBlock(string host_url, Block& b);
void readRawBlocks(string host_url, int startId, int endId, vector<Block>& blocks);
void readRawTransactions(string host_url, vector<Transaction>& transactions);
void readRawHeaders(string host_url, int startId, int endId, vector<BlockHeader>& blockHeaders);
