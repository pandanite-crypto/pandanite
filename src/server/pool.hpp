

// class Pool {
//     void submitWork(string wallet, uint64_t blockId, SHA256Hash nonce) {
//         // check if the block is within current round, if not reject it
//         // check if the nonce is better than best nonce for wallet
//     }

//     void addWorker(string wallet, string ip) {
//         // update division of labor, max one wallet per IP
//     }

//     void startNextBlock() {
//         // count number of workers that submitted at block N-1 and recompute div of labor
//     }

//     void startNextRound() {

//     }

//     void sendPayouts() {

//     }


//     void getWorkForBlock() {

//     }/*
//     This endpoint is polled by workers
//     {
//         "addr1": [0, 2^256],
//         "addr2": [0, 2^256],
//         "addr3": [0, 2^256],
//         "addr4": [0, 2^256],
//         "addr5": [0, 2^256],
//         "addr6": [0, 2^256],
//         "addr7": [0, 2^256],
//         "addr8": [0, 2^256],
//         "addr9": [0, 2^256]
//     }
//     */
// }