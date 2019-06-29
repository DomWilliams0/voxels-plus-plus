#include <boost/thread/thread.hpp>
#include <vector>
#include "threadpool.h"
#include "../lib/loguru/loguru.hpp"

ThreadPool::ThreadPool(unsigned int threads) : workers_(threads), run_(true) {
    if (threads == 0) throw std::runtime_error("thread must be > 0");
    for (unsigned int i = 0; i < threads; ++i) {
        workers_[i] = boost::thread(Worker(*this));
    }
}

ThreadPool::ThreadPool() : ThreadPool(hardware_concurrency()) {}

unsigned int ThreadPool::hardware_concurrency() {
    return boost::thread::hardware_concurrency() * 2;
}

void ThreadPool::shutdown() {
    if (!run_)
        return;

    run_ = false;
    wait_.notify_all();

    for (auto &w : workers_)
        if (w.joinable())
            w.join();

}

ThreadPool::Worker::Worker(ThreadPool &pool) : pool_(pool) {

}

void ThreadPool::Worker::operator()() {
    QueuedTask task;
    bool pop;
    while (pool_.run_) {
        {
            boost::unique_lock<boost::mutex> lock(pool_.wait_mutex_);
            if (pool_.tasks_.empty())
                pool_.wait_.wait(lock);

            pop = pool_.tasks_.pop(task);
        }

        if (pop)
            task();
    }
}
