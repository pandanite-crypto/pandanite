#include <map>
#include <optional>
#include <iostream>
#include <set>
#include <cmath>
#include "../core/block.hpp"
#include "../core/logger.hpp"
#include "../core/helpers.hpp"
#include "executor.hpp"
using namespace std;

void deposit(PublicWalletAddress to, TransactionAmount amt, Ledger& ledger,  LedgerState& deltas) {
    if (!ledger.hasWallet(to)) {
        ledger.createWallet(to);   
    }
    ledger.deposit(to, amt);
    auto receiverDelta = deltas.find(to);
    if (receiverDelta == deltas.end()) {
        deltas.insert(pair<PublicWalletAddress, TransactionAmount>(to, amt));
    } else {
        receiverDelta->second += amt;
    }
}

void withdraw(PublicWalletAddress from, TransactionAmount amt, Ledger& ledger,  LedgerState & deltas) {
    if (ledger.hasWallet(from)) {
        ledger.withdraw(from, amt);
    } else {
        throw std::runtime_error("Tried withdrawing from non-existant account");
    }

    auto senderDelta = deltas.find(from);
    if (senderDelta == deltas.end()) {
        deltas.insert(pair<PublicWalletAddress, TransactionAmount>(from, -amt));
    } else {
        senderDelta->second -= amt;
    }
}

Block Executor::getGenesis() const {
    json genesisJson;
    try {
        genesisJson = readJsonFromFile("genesis.json");
    } catch(...) {
        Logger::logError(RED + "[FATAL]" + RESET, "Could not load genesis.json file.");
        exit(-1);
    }

    Block genesis(genesisJson);
    return genesis;
}

uint32_t computeDifficulty(int32_t currentDifficulty, int32_t elapsedTime, int32_t expectedTime) {
    uint32_t newDifficulty = currentDifficulty;
    if (elapsedTime > expectedTime) {
        int k = 2;
        int lastK = 1;
        while(newDifficulty > MIN_DIFFICULTY) {
                if(abs(elapsedTime/k - expectedTime) > abs(elapsedTime/lastK - expectedTime) ) {
                    break;
                }
            newDifficulty--;
            lastK = k;
            k*=2;
        }
        return newDifficulty;
    } else {
        int k = 2;
        int lastK = 1;
        while(newDifficulty < 254) {
            if(abs(elapsedTime*k - expectedTime) > abs(elapsedTime*lastK - expectedTime) ) {
                break;
            }
            newDifficulty++;
            lastK = k;
            k*=2;
        }
        return newDifficulty;
    }
}

TransactionAmount Executor::getMiningFee(uint64_t blockId) const {
    // compute the fee based on blockId:
    // NOTE:
    // The chain was forked three times, once at 7,750 and again at 125,180, then at 18k
    // Thus we push the chain ahead by this count.
    // SEE: https://bitcointalk.org/index.php?topic=5372707.msg58965610#msg58965610
    uint64_t logicalBlock = blockId + 125180 + 7750 + 18000;
    if (logicalBlock < 1000000) {
        return BMB(50.0);
    } else if (logicalBlock < 2000000) {
        return BMB(25.0);
    }  else if (logicalBlock < 4000000) {
        return BMB(12.5);
    } else {
        return BMB(0.0);
    }
}

int Executor::updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program) const{
    if (numBlocks <= DIFFICULTY_LOOKBACK*2) return initialDifficulty;
    if (numBlocks % DIFFICULTY_LOOKBACK != 0) return initialDifficulty;
    int firstID = numBlocks - DIFFICULTY_LOOKBACK;
    int lastID = numBlocks;  
    Block first = program.getBlock(firstID);
    Block last = program.getBlock(lastID);
    int32_t elapsed = last.getTimestamp() - first.getTimestamp(); 
    uint32_t numBlocksElapsed = lastID - firstID;
    int32_t target = numBlocksElapsed * DESIRED_BLOCK_TIME_SEC;
    int32_t difficulty = last.getDifficulty();
    return computeDifficulty(difficulty, elapsed, target);
}

ExecutionStatus updateLedger(const Transaction& t, PublicWalletAddress& miner, Ledger& ledger, LedgerState & deltas, TransactionAmount blockMiningFee, uint32_t blockId) {
    if (t.isProgramExecution()) {
        ledger.setWalletProgram(t.fromWallet(), t.getProgramId());
        return SUCCESS;
    }
    // check if the wallet has a program execution associated with it:
    if (ledger.hasWalletProgram(t.fromWallet())) {
        return WALLET_LOCKED;
    }
    TransactionAmount amt = t.getAmount();
    TransactionAmount fees = t.getTransactionFee();
    PublicWalletAddress to = t.toWallet();
    PublicWalletAddress from = t.fromWallet();

    if (!t.isFee() && blockId > 1 && walletAddressFromPublicKey(t.getSigningKey()) != t.fromWallet()) {
        return WALLET_SIGNATURE_MISMATCH;
    }
    
    if (t.isFee()) {
        if (amt ==  blockMiningFee) {
            deposit(to, amt, ledger, deltas);
            return SUCCESS;
        } else {
            return INCORRECT_MINING_FEE;
        }
    }

    if (blockId == 1) { // special case genesis block
        deposit(to, amt, ledger, deltas);
    } else {
        // from account must exist
        if (!ledger.hasWallet(from)) {
            return SENDER_DOES_NOT_EXIST;
        }
        TransactionAmount total = ledger.getWalletValue(from);
        // must have enough for amt+fees
        if (total < amt) {
            return BALANCE_TOO_LOW;
        }

        total -= amt;

        if (total < fees) {
            return BALANCE_TOO_LOW;
        }

        withdraw(from, amt, ledger, deltas);
        deposit(to, amt, ledger, deltas);
        if (fees > 0) {
            withdraw(from, fees, ledger, deltas);
            deposit(miner, fees, ledger, deltas);
        }
    }

    return SUCCESS;
}

void rollbackLedger(Transaction& t,  PublicWalletAddress& miner, Ledger& ledger) {
    TransactionAmount amt = t.getAmount();
    TransactionAmount fees = t.getTransactionFee();
    PublicWalletAddress to = t.toWallet();
    PublicWalletAddress from = t.fromWallet();
    if (t.isProgramExecution()) {
        ledger.removeWalletProgram(t.fromWallet());
    } else if (t.isFee()) {
        ledger.revertDeposit(to, amt);
    } else {
        ledger.revertDeposit(to, amt);
        ledger.revertSend(from, amt);
        if (fees > 0) {
            ledger.revertDeposit(miner, fees);
            ledger.revertSend(from, fees);
        }
    }
}

void Executor::rollback(Ledger& ledger, LedgerState& deltas) const{
    for(auto it : deltas) {
        ledger.withdraw(it.first, it.second);
    }
}

void Executor::rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore) const{
    PublicWalletAddress miner;
    for(auto t : curr.getTransactions()) {
        if (t.isFee()) {
            miner = t.toWallet();
            break;
        }
    }
    for(int i = curr.getTransactions().size() - 1; i >=0; i--) {
        Transaction t = curr.getTransactions()[i];
        rollbackLedger(t, miner, ledger);
        if (!t.isFee()) {
            txdb.removeTransaction(t);
        }
    }
    blockStore.removeBlockWalletTransactions(curr);
}

ExecutionStatus Executor::executeTransaction(Ledger& ledger, const Transaction t,  LedgerState& deltas) const{
    if (!t.isFee() && !t.signatureValid()) {
        return INVALID_SIGNATURE;
    }

    if (!t.isFee() && walletAddressFromPublicKey(t.getSigningKey()) != t.fromWallet()) {
        return WALLET_SIGNATURE_MISMATCH;
    }

    PublicWalletAddress miner = NULL_ADDRESS;
    return updateLedger(t, miner, ledger, deltas, BMB(0), 0); // ExecuteTransaction is only used on non-fee transactions
}


ExecutionStatus Executor::executeBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas) const{
    TransactionAmount blockMiningFee = this->getMiningFee(curr.getId());
    // try executing each transaction
    bool foundFee = false;
    PublicWalletAddress miner;
    TransactionAmount miningFee;
    for(auto t : curr.getTransactions()) {
        if (t.isProgramExecution()) {
            if (ledger.hasWalletProgram(t.fromWallet())) {
                return ALREADY_HAS_PROGRAM;
            } else {
                if (walletAddressFromPublicKey(t.getSigningKey()) != t.fromWallet()) {
                    return WALLET_SIGNATURE_MISMATCH;
                }
                continue;
            }
        } if (t.isFee()) {
            if (foundFee) return EXTRA_MINING_FEE;
            miner = t.toWallet();
            miningFee = t.getAmount();
            foundFee = true;
        } else if (txdb.hasTransaction(t) && curr.getId() != 1) {
            return EXPIRED_TRANSACTION;
        }
        
        if (!t.isFee() && curr.getId() > 1 && walletAddressFromPublicKey(t.getSigningKey()) != t.fromWallet()) {
            return WALLET_SIGNATURE_MISMATCH;
        }
    }
    if (!foundFee) {
        return NO_MINING_FEE;
    }

    if (miningFee != blockMiningFee) {
        return INCORRECT_MINING_FEE;
    }
    for(auto t : curr.getTransactions()) {
        if (!t.isFee() && !t.signatureValid() && curr.getId() != 1) {
            return INVALID_SIGNATURE;
        }
        ExecutionStatus updateStatus = updateLedger(t, miner, ledger, deltas, blockMiningFee, curr.getId());
        if (updateStatus != SUCCESS) {
            return updateStatus;
        }
    }
    return SUCCESS;
}

