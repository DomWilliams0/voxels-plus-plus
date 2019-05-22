#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

#include "threadpool.h"

int main() {
    ThreadPool pool;

    auto start = boost::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        pool.post([i]() {
            int x[100];
            for (int j = 0; j < 1000000; ++j) {
                x[j % 100] = j;
            }
        });
    }
//    pool.post_batch_end();

    while (!pool.empty());

    auto end = boost::chrono::steady_clock::now();
    std::cout << boost::chrono::duration_cast<boost::chrono::milliseconds>(end - start) << std::endl;
}