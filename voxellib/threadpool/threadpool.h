#ifndef VOXELS_THREADPOOL_H
#define VOXELS_THREADPOOL_H

#include <functional>
#include <utility>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>

// linked fifo
#include <cstddef>
#include <exception>

#include "spmc/LinkedFIFO.h"

typedef unsigned int Priority;
typedef std::function<void()> QueuedTask;
typedef dkit::spmc::LinkedFIFO<QueuedTask> TaskQueue;

class ThreadPool {
public:
    ThreadPool(unsigned int threads);

    ThreadPool();

    inline ~ThreadPool() { shutdown(); }

    inline bool empty() { return tasks_.empty(); }

    static unsigned int hardware_concurrency();

    // 0 is highest priority
    template<typename T>
    void post(T &&task/*, Priority priority = 10*/) {
        tasks_.push(task);
        wait_.notify_one();
    }

    template<typename T>
    void post_batch(T &&task/*, Priority priority = 10*/) {
        tasks_.push(task);
    }

    inline void post_batch_end() { wait_.notify_all(); }

    void shutdown();

private:
    TaskQueue tasks_;
    boost::condition_variable wait_;
    boost::mutex wait_mutex_;

    std::vector<boost::thread> workers_;
    bool run_;


    struct Worker {
        ThreadPool &pool_;

        Worker(ThreadPool &pool);

        void operator()();

    };
};


#endif
