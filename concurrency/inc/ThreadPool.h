#include "Singleton.h"
#include <future>
#include <vector>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>

/* 线程池特点
 *  1. 并发的，无序的; 有序的任务不适合线程池来做
 *  2. 任务之间互斥或强关联不适合使用线程池
 */

class ThreadPool : public Singleton<ThreadPool> // 歧义递归模板 CRTP
{
    friend class Singleton<ThreadPool>;

public:
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    using Task = std::packaged_task<void()>;

    ThreadPool(unsigned int num = 5)
    {
        if (num < 1)
            thread_num_ = 1;
        else
            thread_num_ = num;

        start();
    }

    ~ThreadPool()
    {
        stop();
    }

    /* 投递任务 */
    template <typename F, typename... Args> /* F 为回调函数， Args 为回调函数所需要的参数 */
    auto commit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
    { /* decltype 推断出 f 函数返回的值的类型并与 future 绑定，通过 .get() 在未来可以获取函数具体返回的值 */
        using RetType = decltype(f(args...));
        if (stop_.load())
        { /* 如果线程池停止了，直接返回一个空的 std::future 对象 */
            return std::future<RetType>{};
        }
        /* 如果线程池没有停止，会往下走正常逻辑 */
        /* std::packaged_task<RetType()> 这里的 RetType() 相当于函数调用，bind 绑定后生成的是无参的函数返回类型是 RetType，然后调用 RetType() 相当于调用有参的 f(args..) */
        /* packages_task 可以绑定回调函数，当我们执行 task 时，相当于在执行 f(args...) */
        auto task = std::make_shared<std::packaged_task<RetType()>>(                                                           /* f(args..) 的返回类型是 RetType 类型 */
                                                                    std::bind(std::forward<F>(f), std::forward<Args>(args)...) /* bind 将函数的参数绑定到函数的内部，生成一个无参函数，新函数的参数是 void */
        );
        std::future<RetType> ret = task->get_future(); /* 获取 f(args...) 任务函数执行后返回的值 */
        {
            std::lock_guard<std::mutex> lk(mutex_);
            /* emplace 中的实际上是一个 task 也是一个构造函数, task 实际上是 packages_task 的智能指针，等价于 std::package_tasks<RetType> task( (*task)() ) 因为 packages_task 的对象可以绑定函数对象 */
            tasks_.emplace([task]
                           { (*task)(); }); /* 将任务投递到任务队列中，当 task 出队的时候，会执行这个 lambda 表达式中指定的回调函数 */
        }
        cond_.notify_one();
        return ret;
    }

private:
    void start()
    {
        for (int i = 0; i < thread_num_; ++i)
        {
            pool_.emplace_back([this]()
                               {
                while (!this->stop_.load()) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lk(mutex_);
                        this->cond_.wait(lk, [this] {
                            return this->stop_.load() || !this->tasks_.empty();
                        });
                        if (this->tasks_.empty()) return;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    this->thread_num_--;
                    task();  /* 执行任务函数 */
                    this->thread_num_++;
                } });
        }
    }

    void stop()
    {
        stop_.store(true);
        cond_.notify_all(); /* 唤醒所有线程 */
        for (auto &td : pool_)
        {
            if (td.joinable())
            {
                std::cout << "thread join " << td.get_id() << std::endl;
                td.join();
            }
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic_int thread_num_;
    std::atomic_bool stop_;
    std::queue<Task> tasks_;
    std::vector<std::thread> pool_;
};