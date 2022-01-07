#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <queue>
#include "crypto.hpp"
#include "api.hpp"
#include "logger.hpp"

#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#define THREAD_PRIORITY_TIME_CRITICAL -15
#define THREAD_PRIORITY_HIGHEST -10
#define THREAD_PRIORITY_ABOVE_NORMAL -5
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_BELOW_NORMAL 5
#define THREAD_PRIORITY_IDLE 19
#endif

#ifndef SCHED_IDLE
#define SCHED_IDLE SCHED_OTHER
#endif

struct Job {
    string host;
    Block newBlock;
};

class Worker
{
public:
    static std::atomic<uint32_t> accepted_blocks;
    static std::atomic<uint32_t> rejected_blocks;
    static std::atomic<uint32_t> earned;

    Worker(int thread_priority) : thread(Worker::start, this), queue(), qmutex(), cond()
    {
        int priority;

        switch (thread_priority) {
        case 5:
            priority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        case 4:
            priority = THREAD_PRIORITY_HIGHEST;
            break;
        case 3:
            priority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case 2:
            priority = THREAD_PRIORITY_NORMAL;
            break;
        case 1:
            priority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        default:
            priority = THREAD_PRIORITY_IDLE;
        }
#if WIN32
        SetThreadPriority(static_cast<HANDLE>(thread.native_handle()), priority);
#else
        int sched;

        if (priority == THREAD_PRIORITY_IDLE) 
        {
            sched = SCHED_IDLE;
        }
        else 
        {
            sched = SCHED_OTHER;
        }

        sched_param sch;
        int policy;
        pthread_getschedparam(thread.native_handle(), &policy, &sch);
        sch.sched_priority = priority;
        pthread_setschedparam(thread.native_handle(), sched, &sch);
#endif
    }

    ~Worker(void)
    {
    }

    void execute(Job job);
    void abandon();

    uint64_t get_hash_count();
private:
    std::queue<Job> queue;

    std::thread thread;
    std::mutex qmutex;
    std::condition_variable cond;

    std::atomic<bool> skip_current = false;
    std::atomic<uint64_t> hash_count = 0;

    static void start(Worker* worker);

    void loop(void);
    void submit(Job& job);
};