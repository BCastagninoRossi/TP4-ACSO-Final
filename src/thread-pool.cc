#include "thread-pool.h"
#include "Semaphore.h"
using namespace std;

Worker::Worker(size_t workerID, ThreadPool* pool) : id(workerID), pool(pool), wsema(0) {
    done = true;
    shouldTerminate = false;
}

void ThreadPool::dispatcher() {
    while (!terminate) {
        sema.wait();
        for (auto& worker : wts) {
            if (worker.done) {
                qmutex.lock();
                if (taskq.empty()) {
                    qmutex.unlock();
                    break;
                }
                worker.thunk = taskq.front();
                taskq.pop();
                qmutex.unlock();
                worker.done = false;
                worker.wsema.signal();
                break;
            }
        }
    }
}

ThreadPool::ThreadPool(size_t numThreads) : sema(0), terminate(false) {
    dt = thread([this] { dispatcher(); });

    for (size_t i = 0; i < numThreads; i++) {
        wts.emplace_back(i, this);
        wts.back().workt = thread([this, i] { work(i); });
    }
}

void ThreadPool::work(size_t workerId) {
    for (auto& worker : wts) {
        if (worker.id == workerId) {
            while (!worker.shouldTerminate) {
                worker.wsema.wait();
                if (worker.shouldTerminate) break;
                worker.thunk();
                worker.done = true;
                sema.signal();
            }
            break;
        }
    }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    qmutex.lock();
    taskq.push(thunk);
    sema.signal();
    qmutex.unlock();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(qmutex);
    while (!taskq.empty()) {
        lock.unlock();
        this_thread::sleep_for(chrono::milliseconds(100));
        lock.lock();
    }
    for (auto& worker : wts) {
        while (!worker.done) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
}

ThreadPool::~ThreadPool() {
    wait();
    terminate = true;
    sema.signal();
    dt.join();
    for (auto& worker : wts) {
        worker.shouldTerminate = true;
        worker.wsema.signal();
        worker.workt.join();
    }
}
