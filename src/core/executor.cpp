#include <map>
#include <optional>
#include <iostream>
#include <set>
#include <cmath>
#include "block.hpp"
#include "executor.hpp"
#include "helpers.hpp"
using namespace std;

string executionStatusAsString(ExecutionStatus status) {
    switch(status) {
        case SENDER_DOES_NOT_EXIST:
            return "SENDER_DOES_NOT_EXIST";
        break;
        case BALANCE_TOO_LOW:
            return "BALANCE_TOO_LOW";
        break;
        case INVALID_SIGNATURE:
            return "INVALID_SIGNATURE";
        break;
        case INVALID_NONCE:
            return "INVALID_NONCE";
        break;
        case EXTRA_MINING_FEE:
            return "EXTRA_MINING_FEE";
        break;
        case INCORRECT_MINING_FEE:
            return "INCORRECT_MINING_FEE";
        break;
        case INVALID_BLOCK_ID:
            return "INVALID_BLOCK_ID";
        break;
        case NO_MINING_FEE:
            return "NO_MINING_FEE";
        break;
        case UNKNOWN_ERROR:
            return "UNKNOWN_ERROR";
        break;
        case INVALID_TRANSACTION_NONCE:
            return "INVALID_TRANSACTION_NONCE";
        break;
        case INVALID_DIFFICULTY:
            return "INVALID_DIFFICULTY";
        break;
        case INVALID_TRANSACTION_TIMESTAMP:
            return "INVALID_TRANSACTION_TIMESTAMP";
        break;
        case SUCCESS:
            return "SUCCESS";
        break;
        case QUEUE_FULL:
            return "QUEUE_FULL";
        break;
        case EXPIRED_TRANSACTION:
            return "EXPIRED_TRANSACTION";
        break;
        case BLOCK_ID_TOO_LARGE:
            return "BLOCK_ID_TOO_LARGE";
        break;
    }
}

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

ExecutionStatus updateLedger(Transaction& t, Ledger& ledger, LedgerState & deltas) {
    TransactionAmount amt = t.getAmount();
    TransactionAmount fees = t.getTransactionFee();
    PublicWalletAddress to = t.toWallet();
    PublicWalletAddress from = t.fromWallet();
    PublicWalletAddress miner;
    
    if (t.isFee()) {
        if (amt ==  MINING_FEE) {
            deposit(to, amt, ledger, deltas);
            return SUCCESS;
        } else {
            return INCORRECT_MINING_FEE;
        }
    }
    
    // from account must exist
    if (!ledger.hasWallet(from)) {
        return SENDER_DOES_NOT_EXIST;
    }
    TransactionAmount total = ledger.getWalletValue(from);
    // must have enough for amt+fees
    if (total < (amt + fees)) {
        return BALANCE_TOO_LOW;
    }
    withdraw(from, amt, ledger, deltas);
    deposit(to, amt, ledger, deltas);
    if (fees > 0 && t.hasMiner()) {
        miner = t.getMinerWallet();
        withdraw(from, fees, ledger, deltas);
        deposit(miner, fees, ledger, deltas);
    }
    return SUCCESS;
}

void rollbackLedger(Transaction& t, Ledger& ledger) {
    TransactionAmount amt = t.getAmount();
    TransactionAmount fees = t.getTransactionFee();
    PublicWalletAddress to = t.toWallet();
    PublicWalletAddress from = t.fromWallet();
    PublicWalletAddress miner;
    
    if (t.isFee()) {
        ledger.revertDeposit(to, amt);
    } else {
        ledger.revertDeposit(to, amt);
        ledger.revertSend(from, amt);
        if (fees > 0 && t.hasMiner()) {
            miner = t.getMinerWallet();
            ledger.revertDeposit(miner, fees);
            ledger.revertSend(from, fees);
        }
    }
}

void Executor::Rollback(Ledger& ledger, LedgerState& deltas) {
    for(auto it : deltas) {
        ledger.withdraw(it.first, it.second);
    }
}

void Executor::RollbackBlock(Block& curr, Ledger& ledger) {
    for(int i = curr.getTransactions().size() - 1; i >=0; i--) {
        Transaction t = curr.getTransactions()[i];
        rollbackLedger(t, ledger);
    }
}

ExecutionStatus Executor::ExecuteTransaction(Ledger& ledger, Transaction t, LedgerState& deltas) {
    if (!t.isFee() && !t.signatureValid()) {
        return INVALID_SIGNATURE;
    }
    if (t.isFee()) {
        return EXTRA_MINING_FEE; 
    }
    return updateLedger(t, ledger, deltas);
}

ExecutionStatus Executor::ExecuteBlock(Block& curr, Ledger& ledger, LedgerState& deltas) {
    // try executing each transaction
    bool foundFee = false;
    set<string> nonces;

    for(auto t : curr.getTransactions()) {
        if (nonces.find(t.getNonce())!= nonces.end()) {
            return INVALID_TRANSACTION_NONCE;
        } else {
            nonces.insert(t.getNonce());
        }
        if (!t.isFee() && !t.signatureValid()) {
            return INVALID_SIGNATURE;
        }
        if (t.getBlockId() != curr.getId()) {
            return INVALID_BLOCK_ID;
        }
        if (t.isFee()) {
            if (foundFee || t.getBlockId() >= MINING_PAYMENTS_UNTIL) {
                return EXTRA_MINING_FEE;
            }
            foundFee = true;
        }
        ExecutionStatus updateStatus = updateLedger(t, ledger, deltas);
        if (updateStatus != SUCCESS) {
            return updateStatus;
        }
    }
    if (!foundFee && curr.getId() < MINING_PAYMENTS_UNTIL) {
        return NO_MINING_FEE;
    }
    return SUCCESS;
}

