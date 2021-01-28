#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace huhu {

class ThreadPool{
public:
    using TaskFunc = std::function<void()>;

    ThreadPool(int workers);
    ~ThreadPool();
    void addTask(const TaskFunc& task);

private:
    std::vector<std::thread> m_threads;
    std::mutex m_lock;
    std::condition_variable m_cond;
    std::queue<TaskFunc> m_tasks;
    bool m_shutdown;
};

}


#endif