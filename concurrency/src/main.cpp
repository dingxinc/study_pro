#include "../inc/threadsafe_queue.h"
#include "../inc/ThreadPool.h"
#include "../inc/QuickSort.h"

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

int main()
{
    // test_safe_queue();
    // test_thread_pool();
    test_quick_sort();
    return 0;
}