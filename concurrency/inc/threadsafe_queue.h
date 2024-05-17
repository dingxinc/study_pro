#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

template <typename T>
class threadsafe_queue
{
private:
    mutable std::mutex _mutex; /* mutable 表示即使有 const 限定，也可以对值进行修改 */
    std::queue<T> _queue;
    std::condition_variable _cond;

public:
    threadsafe_queue() {}
    threadsafe_queue(threadsafe_queue const &other)
    {                                                 /* 当没有声明移动构造时，const& 可以被移动构造调用 */
        std::lock_guard<std::mutex> lk(other._mutex); /* 加锁 */
        _queue = other._queue;
    }

    void push(T value)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _queue.push(value);
        _cond.notify_one(); /* 其他线程消费队列的时候发现队列为空挂起了，我们 push 完队列有数据了，通知挂起的线程可以消费了 */
    }

    /* 阻塞的 pop */
    void wait_and_pop(T &value) /* 传递引用外面的值同样会被修改 */
    {                           /* 当队列为空的时候会等待，当队列不为空的时候才 pop */
        std::unique_lock<std::mutex> lk(_mutex);
        _cond.wait(lk, [this]
                   { return !_queue.empty(); }); /* this 捕获这个类，可以使用你这个类所有成员和函数 当这个谓词返回 false 的时候，wait 就会卡在这里等待其他线程唤醒，队列为空，就在这里挂起 */
        /* 当其他线程 notify 后，队列非空 */
        value = _queue.front(); /* 因为 value 是 T 类型的引用，只在这里发生一次拷贝 */
        _queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(_mutex);
        _cond.wait(lk, [this]
                   { return !_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
        _queue.pop();
        return res; /* 返回一个局部的指针或引用是危险的，但是智能指针不会有这个问题 */
    }

    /* 非阻塞的 pop */
    bool try_pop(T &value)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_queue.empty())
            return false; /* 队列为空，直接返回，非阻塞方式效率高 */
        value = _queue.front();
        _queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_queue.empty())
            return std::make_shared<T>(); /* 队列为空返回空指针 */
        std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
        _queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(_mutex);
        return _queue.empty();
    }
};

/****************************************************************************/
#include <thread>
#include <chrono>
#include <iostream>

void test_safe_queue()
{
    threadsafe_queue<int> safe_queue;
    std::mutex mtx_print;
    std::thread producer(
        [&]()
        {
            for (int i = 0;; i++)
            {
                safe_queue.push(i);
                {
                    std::lock_guard<std::mutex> lk(mtx_print);
                    std::cout << "producer push data is " << i << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });

    std::thread consumer1(
        [&]()
        {
            for (;;)
            {
                auto data = safe_queue.wait_and_pop();
                {
                    std::lock_guard<std::mutex> lk(mtx_print);
                    std::cout << "consumer1 pop data is " << *data << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });

    std::thread consumer2(
        [&]()
        {
            for (;;)
            {
                auto data = safe_queue.try_pop();
                if (data != nullptr)
                {
                    {
                        std::lock_guard<std::mutex> lk(mtx_print);
                        std::cout << "consumer2 pop data is " << *data << std::endl;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });

    producer.join();
    consumer1.join();
    consumer2.join();
}