#include <map>
#include <iostream>
#include <emscripten/emscripten.h>
using namespace std;


// https://gist.github.com/WesThorburn/00c47b267a0e8c8431e06b14997778e4


#ifdef __cplusplus
extern "C" {
#endif


// Websocket in JS will receive requests from other nodes
// and pass them through this function, replying with response
EMSCRIPTEN_KEEPALIVE const char* sendRequest(char* st) {
    string str = string(st);
    // json data = json::parse(str);
    // string endpoint = data["endpoint"];
    // TODO: support these end points
    // .get("/block_count", blockCountHandler)
    // .get("/block/:b", blockHandler)
    // .get("/ledger/:user", ledgerHandler)
    // .get("/total_work", totalWorkHandler)
    // .post("/add_peer", addPeerHandler)
    // .get("/tx_json", txJsonHandler)
    // .get("/mine_status/:b", mineStatusHandler)
    // .get("/mine", mineHandler)
    // .post("/submit", submitHandler)
    // .get("/gettx/:blockId", getTxHandler)
    // .get("/gettx", getTxHandler)
    // .get("/sync/:start/:end", syncHandler)
    // .get("/block_headers/:start/:end", blockHeaderHandler)
    // .get("/synctx", getTxHandler)
    // .post("/add_transaction", addTransactionHandler)
    // .post("/add_transaction_json", addTransactionJSONHandler)
    // .post("/verify_transaction", verifyTransactionHandler)
    return 0;
}


#ifdef __cplusplus
}
#endif

int main(int argc, char** argv) {
    return 0;
}