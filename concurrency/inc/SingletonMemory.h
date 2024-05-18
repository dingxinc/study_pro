#include <mutex>
#include <memory>
#include <atomic>
#include <iostream>

// 利用内存模型改良单例模式
class SingleMemoryModel
{
private:
    SingleMemoryModel()
    {
    }
    SingleMemoryModel(const SingleMemoryModel &) = delete;
    SingleMemoryModel &operator=(const SingleMemoryModel &) = delete;

public:
    ~SingleMemoryModel()
    {
        std::cout << "single auto delete success " << std::endl;
    }
    static std::shared_ptr<SingleMemoryModel> GetInst()
    {
        // 1 处
        if (_b_init.load(std::memory_order_acquire))
        {
            return single;
        }
        // 2 处
        s_mutex.lock();
        // 3 处
        if (_b_init.load(std::memory_order_relaxed))
        {
            s_mutex.unlock();
            return single;
        }
        // 4处
        single = std::shared_ptr<SingleMemoryModel>(new SingleMemoryModel);
        _b_init.store(true, std::memory_order_release);
        s_mutex.unlock();
        return single;
    }

private:
    static std::shared_ptr<SingleMemoryModel> single;
    static std::mutex s_mutex;
    static std::atomic<bool> _b_init;
};

std::shared_ptr<SingleMemoryModel> SingleMemoryModel::single = nullptr;
std::mutex SingleMemoryModel::s_mutex;
std::atomic<bool> SingleMemoryModel::_b_init = false;

/* 测试 */
void TestSingleMemory()
{
    std::thread t1([]()
                   { std::cout << "thread t1 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl; });

    std::thread t2([]()
                   { std::cout << "thread t2 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl; });

    t2.join();
    t1.join();
}