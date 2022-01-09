#include "worker.hpp"
#include "helpers.hpp"

std::atomic<uint32_t> Worker::accepted_blocks;
std::atomic<uint32_t> Worker::rejected_blocks;
std::atomic<uint32_t> Worker::earned;

void Worker::execute(Job job)
{
    {
        std::lock_guard<std::mutex> lock(qmutex);
        queue.push(std::move(job));
    }
    cond.notify_one();
}

void Worker::abandon()
{
    skip_current.store(true);
}

uint64_t Worker::get_hash_count()
{
    return hash_count.load();
}

void Worker::start(Worker* worker)
{
    while (true) {
        worker->loop();
    }
}

void Worker::loop(void)
{
    Job job;

    {
        std::unique_lock<std::mutex> lock(qmutex);
        while (queue.empty()) { cond.wait(lock); }
        job = queue.front();
        queue.pop();
    }

    skip_current.store(false);

    SHA256Hash solution = mineHash(job.newBlock.getHash(), job.newBlock.getDifficulty(),
        [this](int count) {
            this->hash_count.fetch_add(count);
        },
        [this]() {
            return skip_current.load();
    });

    if (solution != NULL_SHA256_HASH) {
        job.newBlock.setNonce(solution);
        try {
            submit(job);
        } catch(...) {
            Logger::logStatus(RED + "[ SUBMIT FAILED ] " + RESET);
        }
        
    }
}

void Worker::submit(Job& job) {
    auto transmit_start = std::chrono::steady_clock::now();
    auto result = submitBlock(job.host, job.newBlock);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - transmit_start).count();

    if (result.contains("status") && string(result["status"]) == "SUCCESS") {
        Worker::earned.fetch_add(50); // TODO: remove hardcoded
        auto accepted_blocks = Worker::accepted_blocks.fetch_add(1) + 1;
        auto rejected_blocks = Worker::rejected_blocks.load();
        auto total_blocks = accepted_blocks + rejected_blocks;
        Logger::logStatus(GREEN + "[ ACCEPTED ] " + RESET + to_string(accepted_blocks) + " / " + to_string(rejected_blocks) + " (" + to_string(accepted_blocks / (double)total_blocks * 100) + ") " + to_string(elapsed) + "ms");
    }
    else {
        auto accepted_blocks = Worker::accepted_blocks.load();
        auto rejected_blocks = Worker::rejected_blocks.fetch_add(1) + 1;
        auto total_blocks = accepted_blocks + rejected_blocks;
        Logger::logStatus(RED + "[ REJECTED ] " + RESET + to_string(accepted_blocks) + " / " + to_string(rejected_blocks) + " (" + to_string(rejected_blocks / (double)total_blocks * 100) + ") " + to_string(elapsed) + "ms");
        Logger::logStatus(result.dump(4));
    }
}