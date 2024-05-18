#include "../inc/threadsafe_queue.h"
#include "../inc/ThreadPool.h"

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

int main()
{
    // test_safe_queue();
    int m = 0;
    ThreadPool::getInstance()->commit([](int &m)
                                      {
		m = 1024;
		std::cout << "inner set m is " << m << std::endl; }, m);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "m is " << m << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    ThreadPool::getInstance()->commit([](int &m)
                                      {
		m = 1024;
		std::cout << "inner set m is " << m << std::endl; }, std::ref(m));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "m is " << m << std::endl;
    return 0;
}