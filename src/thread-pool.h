#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <list>      // for list
#include <mutex>       // for mutex
#include <queue>       // for queue

#include "Semaphore.h"

class ThreadPool;

struct Worker {
    Worker(size_t workerID, ThreadPool* pool);
    size_t id;
    std::thread workt;
    bool done;
    std::function<void(void)> thunk;
    ThreadPool* pool;
    Semaphore wsema;
    bool shouldTerminate;
};

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    void schedule(const std::function<void(void)>& thunk);
    void work(size_t workerId);
    void wait();
    ~ThreadPool();

private:
    std::thread dt;                // dispatcher thread handle
    std::list<Worker> wts;       // Worker instances
    std::queue<std::function<void(void)>> taskq;  // task queue

    Semaphore sema;
    std::mutex qmutex;
    bool terminate;

    void dispatcher();

    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif
