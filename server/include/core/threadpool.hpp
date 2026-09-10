#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <condition_variable>

namespace tecm{

class thread_pool;

template<typename T>
struct future
{
private:
    friend class thread_pool;
    T val;
    bool ready = false;
    std::condition_variable cv_;
    std::mutex mtx_;
public:
    T& get()
    {
        std::unique_lock<std::mutex> ul{mtx_};
        cv_.wait(ul, [&]{
            return ready;
        });
        return val;
    }

};

class thread_pool
{
private:
    const uint32_t threadCount_ = std::thread::hardware_concurrency() == 0 ? 2 : std::thread::hardware_concurrency();

    std::vector<std::function<void()>> workPool_;
    std::vector<std::jthread> threadPool_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool running_ = true;

    template<typename T_Future_v>
    class future_detail
    {
    private:
        future<T_Future_v>& future_;
    public:

        future_detail(future<T_Future_v>& future) : future_(future) {  }
        T_Future_v& val()
        { return future_.val; }
        void makeready()
        {
            std::scoped_lock<std::mutex> lg{future_.mtx_};
            future_.ready = true;
            future_.cv_.notify_all();
        }
    };

public:
    uint32_t threadcount() { return threadCount_; }

    int init();
    int cleanup();

    template <typename Func>
    requires std::is_invocable_r_v<void, Func>
    int32_t assign_work(Func&& work)
    {
        std::scoped_lock<std::mutex> lg{mtx_};
        workPool_.push_back(work);
        cv_.notify_one();
        return 0;
    }

    int32_t assign_many(const std::vector<std::function<void()>>& work);

    template <typename Func>
    requires std::is_invocable_r_v<void, Func>
    int32_t block_for_work(Func&& work) // this is kinda useless... maybe calling other threads in this thread? kinda ub for me tho...
    {
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        assign_work([work, &ready, &mtx, &cv]{ work(); std::scoped_lock<std::mutex> lg{mtx}; ready = true; cv.notify_one(); });
        std::unique_lock<std::mutex> ul{mtx};
        cv.wait(ul, [&ready]{ return ready; });
        return 0;
    }

    int32_t block_for_many(const std::vector<std::function<void()>>& work);

    template <typename T_Future_v, typename Func>
    requires std::is_invocable_r_v<T_Future_v, Func>
    void assign_future(future<T_Future_v>& future, Func&& work)
    {
        assign_work([&future, work]{
            future_detail<T_Future_v> detail{future};
            detail.val() = work();
            detail.makeready();
        });
    }

    template <typename T_Container, typename Func>
    requires std::is_invocable_r_v<void, Func, uint32_t, typename T_Container::value_type&>
    //&& util::IterableContainer<T_Container>
    int32_t block_for_parse(T_Container& array, Func&& parseWork)
    {
        if (array.empty()) return -1;
        uint32_t targetThreadCount = threadCount_;
        if (array.size() < targetThreadCount)
        {
            for (size_t i = 0; i < array.size(); ++i) { parseWork(0, array[i]); }
            return 1;
        }
        std::vector<std::pair<uint32_t, uint32_t>> startEndIndices(targetThreadCount);
        uint32_t sliceSize = array.size() / targetThreadCount;
        uint32_t remainder = array.size() - sliceSize * targetThreadCount;
        for (uint32_t i = 0; i < targetThreadCount; i++)
        {
            uint32_t start = i * sliceSize;
            uint32_t end = sliceSize * (i + 1);
            end = i == targetThreadCount - 1 ? end + remainder : end;
            startEndIndices[i] = { start, end };
        }
        std::vector<std::function<void()>> work;
        uint32_t j = 0;
        for (auto& indices : startEndIndices)
        {
            work.push_back(
                [&, j, indices]{
                    for (int i = indices.first; i < indices.second; i++)
                    {
                        parseWork(j, array[i]);
                    }
                }
            );
            j++;
        }
        block_for_many(work);
        return 0;
    }

};

}
