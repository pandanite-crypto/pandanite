#include "program.hpp"
#include "executor.hpp"
#include "wasm_executor.hpp"

Program::Program(vector<uint8_t>& byteCode, json config) {
    this->byteCode = byteCode;
    this->id = SHA256((char*)byteCode.data(), byteCode.size());
    this->executor = std::make_shared<WasmExecutor>(this->byteCode);
    this->init(config);
}

Program::Program(json obj, json config){
    this->byteCode = hexDecode(obj["byteCode"]);
    this->id = SHA256((char*)byteCode.data(), byteCode.size());
    std::string basePath = config["storagePath"];
    this->executor = std::make_shared<WasmExecutor>(this->byteCode);
    this->init(config);
}

Program::Program(json config){
    this->id = NULL_SHA256_HASH;
    // use the default executor
    this->executor = std::make_shared<Executor>();
    this->init(config);
}

void Program::init(json config) {
    this->lastHash = NULL_SHA256_HASH;
    this->difficulty = this->getGenesis().getDifficulty();
    std::string basePath = config["storagePath"];
    basePath += SHA256toString(this->id).substr(0,5);
    basePath += "_";
    this->ledger.init(basePath + "ledger");
    this->blockStore.init(basePath + "blocks");
    this->txdb.init(basePath + "txdb");
    this->state.init(basePath + "state");
    if (this->blockStore.hasBlockCount()) {
        this->numBlocks = this->blockStore.getBlockCount();
        this->totalWork = this->blockStore.getTotalWork();
        Block last = this->blockStore.getBlock(this->numBlocks);
        this->lastHash = last.getHash();
        this->difficulty = last.getDifficulty();
    }
}

vector<uint8_t> Program::getByteCode() const {
    return this->byteCode;
}

json Program::toJson() const{
    json ret;
    ret["byteCode"] = hexEncode((char*)this->byteCode.data(), this->byteCode.size());
    return ret;
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
    this->lastHash = NULL_SHA256_HASH;
    this->difficulty = this->getGenesis().getDifficulty();
    this->numBlocks = 0;
    this->totalWork = 0;
    this->ledger.clear();
    this->blockStore.clear();
    this->txdb.clear();
}

void Program::closeDB() {
    txdb.closeDB();
    ledger.closeDB();
    blockStore.closeDB();
    state.closeDB();
}

void Program::deleteDB() {
    this->closeDB();
    txdb.deleteDB();
    ledger.deleteDB();
    blockStore.deleteDB();
    state.deleteDB();
}

Block Program::getGenesis() const {
    return this->executor->getGenesis();
}

bool Program::hasBlockCount() const {
    return this->blockStore.hasBlockCount();
}

BlockStore* Program::getBlockstore() {
    return &this->blockStore;
}

uint64_t Program::getBlockCount() const {
    if (!this->hasBlockCount()) return 0;
    return this->blockStore.getBlockCount();
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

Bigint Program::getTotalWork() const {
    return this->blockStore.getTotalWork();
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

int Program::getDifficulty() const {
    return this->difficulty;
}

SHA256Hash Program::getLastHash() const {
    return this->lastHash;
}

vector<SHA256Hash> Program::getTransactionsForWallet(const PublicWalletAddress& wallet) const {
    return this->blockStore.getTransactionsForWallet(wallet);
}

TransactionAmount Program::getCurrentMiningFee() const {
    return this->executor->getMiningFee(this->numBlocks);
}

json Program::getInfo(json args) {
    return this->executor->getInfo(args, this->state);
}

ExecutionStatus Program::executeBlock(Block& block) {
    LedgerState deltasFromBlock;
    ExecutionStatus status = this->executor->executeBlock(block, this->ledger, this->txdb, deltasFromBlock);
    if (status != SUCCESS) {
        this->rollback(deltasFromBlock);
    } else {
        // add all transactions to txdb:
        for(auto t : block.getTransactions()) {
            this->txdb.insertTransaction(t, block.getId());
        }
        this->blockStore.setBlock(block);
        this->numBlocks++;
        this->totalWork = addWork(this->totalWork, block.getDifficulty());
        this->blockStore.setTotalWork(this->totalWork);
        this->blockStore.setBlockCount(this->numBlocks);
        this->lastHash = block.getHash();
        this->difficulty = this->executor->updateDifficulty(this->difficulty, this->numBlocks, *this);
    }
    return status;
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

uint64_t Program::blockForTransaction(const Transaction& t) const {
    return this->txdb.blockForTransaction(t);
}

void Program::rollbackBlock(Block& block) {
    this->executor->rollbackBlock(block, this->ledger, this->txdb, this->blockStore);
    this->numBlocks--;
    Block last = this->getBlock(this->numBlocks);
    Bigint base = 2;
    this->totalWork -= base.pow((int)last.getDifficulty());
    this->difficulty = last.getDifficulty();
    this->blockStore.setTotalWork(totalWork);
    this->blockStore.setBlockCount(this->numBlocks);
}

void Program::setTotalWork(Bigint total) {
    this->blockStore.setTotalWork(total);
}

void Program::setBlockCount(uint64_t count) {
    this->blockStore.setBlockCount(count);
}

void Program::removeBlockWalletTransactions(Block& b) {
    this->blockStore.removeBlockWalletTransactions(b);
}

void Program::deleteData() {
    this->blockStore.closeDB();
    this->txdb.closeDB();
    this->ledger.closeDB();
    this->state.closeDB();
    this->blockStore.deleteDB();
    this->txdb.deleteDB();
    this->ledger.deleteDB();
    this->state.deleteDB();
}

bool operator==(const Program& a, const Program& b) {
    if(a.id != b.id) return false;
    return true;
}