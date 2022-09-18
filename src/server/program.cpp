#include "program.hpp"

Program::Program(vector<uint8_t>& byteCode, HostManager& hosts) : hosts(hosts){
    this->byteCode = byteCode;
    this->id = SHA256((char*)byteCode.data(), byteCode.size());
    //this->executor = std::make_shared<WasmExecutor>(this->byteCode);
}

Program::Program(json obj, HostManager& hosts) : hosts(hosts){
    this->byteCode = hexDecode(obj["byteCode"]);
    this->id = SHA256((char*)byteCode.data(), byteCode.size());
    //this->executor = std::make_shared<WasmExecutor>(this->byteCode);
}

Program::Program(HostManager& hosts) : hosts(hosts){
    this->id = NULL_SHA256_HASH;
    // use the default executor
    this->executor = std::make_shared<Executor>();
}

vector<uint8_t> Program::getByteCode() const {
    return this->byteCode;
}

json Program::toJson() const{
    json obj;
    obj["byteCode"] = hexEncode((char*)this->byteCode.data(), this->byteCode.size());
    return obj;
}

bool Program::hasWalletProgram(PublicWalletAddress& wallet) const {
    return this->ledger.hasWalletProgram(wallet);
}

ProgramID Program::getWalletProgram(PublicWalletAddress& wallet) const {
    return this->ledger.getWalletProgram(wallet);
}

ProgramID Program::getId() const{
    return this->id;
}

void Program::clearState() {
    this->ledger.clear();
    this->blockStore.clear();
    this->txdb.clear();
}

void Program::closeDB() {
    txdb.closeDB();
    ledger.closeDB();
    blockStore.closeDB();
}

void Program::deleteDB() {
    this->closeDB();
    txdb.deleteDB();
    ledger.deleteDB();
    blockStore.deleteDB();
}

Block Program::getGenesis() const {
    return this->executor->getGenesis();
}

uint32_t Program::blockForTransactionId(SHA256Hash txid) const {
    return this->txdb.blockForTransactionId(txid);
}

Block Program::getBlock(uint64_t blockId) const {
    return this->blockStore.getBlock(blockId);
}

std::pair<uint8_t*, size_t> Program::getRawBlock(uint64_t blockId) const {
    return this->blockStore.getRawData(blockId);
}

BlockHeader Program::getBlockHeader(uint64_t blockId) const {
    return this->blockStore.getBlockHeader(blockId);
}

TransactionAmount Program::getWalletValue(PublicWalletAddress& wallet) const {
    return this->ledger.getWalletValue(wallet);
}

bool Program::hasTransaction(Transaction& t) const {
    return this->txdb.hasTransaction(t);
}

ExecutionStatus Program::executeBlock(Block& block, LedgerState& deltasFromBlock) {
    this->executor->executeBlock(block, this->ledger, this->txdb, deltasFromBlock);
}

ExecutionStatus Program::executeTransaction(const Transaction& t, LedgerState& deltas) {
    if (this->txdb.hasTransaction(t)) {
        return EXPIRED_TRANSACTION;
    }
    return this->executor->executeTransaction(this->ledger, t, deltas);
}

void Program::rollback(LedgerState& deltas) {
    this->executor->rollback(this->ledger, deltas);
}

void Program::rollbackBlock(Block& block) {
    this->executor->rollbackBlock(block, this->ledger, this->txdb);
}

void Program::setTotalWork(Bigint total) {
    this->blockStore.setTotalWork(this->totalWork);
}

void Program::setBlockCount(uint64_t count) {
    this->blockStore.setBlockCount(this->numBlocks);
}

void Program::removeBlockWalletTransactions(Block& b) {
    this->blockStore.removeBlockWalletTransactions(b);
}

bool operator==(const Program& a, const Program& b) {
    if(a.id != b.id) return false;
    return true;
}