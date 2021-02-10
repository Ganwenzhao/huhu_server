#include "../include/ThreadPool.h"

#include <iostream>
#include <cassert>

using namespace huhu;

ThreadPool::ThreadPool(int workers)
    : m_shutdown(false){
    workers = workers <= 0 ? 1 : workers;
    for(int i = 0; i < workers; ++i)
    //lambda expr
        m_threads.emplace_back([this]() {
            while(1) {
                TaskFunc func;
                {
                    std::unique_lock<std::mutex> lock(m_lock);    
                    while(!m_shutdown && m_tasks.empty())
                        m_cond.wait(lock); 
                    if(m_tasks.empty() && m_shutdown) {
                        return;
                    }

                    func = m_tasks.front();
                    m_tasks.pop();
                }
                if(func) {
                    func();
                } 
            }
        });
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_shutdown = true;
    } 
    //notify threads kill themselves
    m_cond.notify_all();
    for(auto& thread: m_threads)
        thread.join();
}

void ThreadPool::addTask(const TaskFunc& task)
{
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_tasks.push(task);
    }
    m_cond.notify_one();
}
