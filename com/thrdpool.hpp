#pragma once
#include <cassert>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <functional>

namespace server_utils {
using namespace std;

class event {
    mutex mux;
    condition_variable cond;
    bool signaled;
    bool auto_reset;
public:
    event(bool init_signaled = false, bool auto_reset = true) : signaled(init_signaled), auto_reset(auto_reset) {
    }

    void wait(void) {
        unique_lock<mutex> lock(mux);
        while (!signaled) {
            cond.wait(lock);
        }
        if (auto_reset) {
            reset();
        }
    }

    void signal(void) {
        scoped_lock lock(mux);
        signaled = true;
        cond.notify_one();
    }

    void signal_all(void) {
        scoped_lock lock(mux);
        signaled = true;
        cond.notify_all();
    }

    void reset(void) {
        signaled = false;
    }
};

class semaphore {
    mutex mux;
    condition_variable cond;
    unsigned long count;

public:
    semaphore(unsigned long init_count = 0) : count(init_count) {
    }

    void release(void) {
        scoped_lock<mutex> lock(mux);
        count++;
        cond.notify_one();
    }

    void acquire(void) {
        unique_lock<mutex> lock(mux);
        while(!count) {
            cond.wait(lock);
        }
        count--;
    }

    bool try_acquire() {
        scoped_lock<mutex> lock(mux);
        if(count) {
            --count;
            return true;
        }
        return false;
    }
};

class thread_pool {
    using task_t = function<void(void)>;
    semaphore task_count;
    queue<task_t> task_queue;
    mutex task_queue_lock;
    vector<thread> workers;
public:
    thread_pool(int thread_count = 0) {
        if (0 == thread_count) {
            thread_count = thread::hardware_concurrency() + 2;
        }

        workers.resize(thread_count);

        for (size_t i = 0; i < thread_count; i++) {
            workers[i] = thread([this](){
                while(true) {
                    task_count.acquire();

                    task_t task; {
                        scoped_lock lock(task_queue_lock);

                        if (task_queue.empty()) break;

                        task = move(task_queue.front());
                        task_queue.pop();
                    }

                    try {
                        task();
                    } catch (...) {
                    }
                }
            });
        }
    }

    void execute(task_t new_task) {
        scoped_lock lock(task_queue_lock);
        task_queue.push(new_task);
        task_count.release();
    }

    ~thread_pool() {
        {
            scoped_lock lock(task_queue_lock);
            decltype(task_queue) empty;
            task_queue.swap(empty);
            for (size_t i = 0; i < workers.size(); i++)
                task_count.release();
        }
        for (auto &worker : workers) {
            worker.join();
        }
    }
};
    
} // namespace threadpool
