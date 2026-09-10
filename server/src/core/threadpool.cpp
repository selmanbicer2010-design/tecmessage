#include "core/threadpool.hpp"

using namespace tecm;

int thread_pool::init()
{
    threadPool_.reserve(threadCount_);
    for (int i = 0; i < threadCount_; i++)
    {
        threadPool_.push_back({std::jthread([this, i](){
            while (true)
            {
                std::unique_lock<std::mutex> ul{mtx_};
                cv_.wait(ul, [this]{ return workPool_.size() != 0 or !running_; });
                if (workPool_.size() == 0 and !running_)
                { return; }
                else
                {
                    auto work = workPool_.back();
                    workPool_.pop_back();
                    ul.unlock();
                    work();
                }
            }
        })
        });
    }
    return 0;
}

int thread_pool::cleanup()
{
    mtx_.lock();
    running_ = false;
    cv_.notify_all();
    mtx_.unlock();
    for (auto& thread : threadPool_)
    {
        if (thread.joinable()) thread.join();
    }
    threadPool_.clear();
    workPool_.clear();
    return 0;
}

int32_t thread_pool::assign_many(const std::vector<std::function<void()>>& work)
{
    for (int i = 0; i < work.size(); i++)
    {
        assign_work([work, i]{ work[i](); });
    }
    return 0;
}

int32_t thread_pool::block_for_many(const std::vector<std::function<void()>>& work)
{
    std::mutex mtx;
    std::condition_variable cv;
    uint32_t remaining = work.size();
    for (int i = 0; i < work.size(); i++)
    {
        assign_work([work, i, &remaining, &mtx, &cv]{ work[i](); std::scoped_lock<std::mutex> lg{mtx}; remaining--; cv.notify_one(); });
    }
    std::unique_lock<std::mutex> ul{mtx};
    cv.wait(ul, [&remaining]{ return remaining == 0; });
    return 0;
}
