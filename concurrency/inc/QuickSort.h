#include <iostream>
#include "ThreadPool.h"

// 快速排序（Quick Sort）是一种高效的排序算法，采用分治法的思想进行排序。以下是快速排序的基本步骤：

// 1. 选择一个基准元素（pivot）：从数组中选择一个元素作为基准元素。选择基准元素的方式有很多种，常见的是选择数组的第一个元素或最后一个元素。
// 2. 分区（partitioning）：重新排列数组，把比基准元素小的元素放在它的左边，把比基准元素大的元素放在它的右边。在这个过程结束时，基准元素就处于数组的最终位置。
// 3. 递归排序子数组：递归地对基准元素左边和右边的两个子数组进行快速排序。

/* 递归快速排序 */
template <typename T>
void quick_sort_recursive(T arr[], int start, int end)
{
    if (start >= end)
        return;
    T key = arr[start]; /* 第一个元素作为基准元素 */
    int left = start, right = end;
    while (left < right)
    {
        while (arr[right] >= key && (left < right))
            right--;
        while (arr[left] <= key && (left < right))
            left++;
        std::swap(arr[left], arr[right]);
    }

    if (arr[left] < key)
    {
        std::swap(arr[left], arr[start]);
    }

    quick_sort_recursive(arr, start, left - 1); /* 递归排左边的 */
    quick_sort_recursive(arr, left + 1, end);   /* 递归排右边的 */
}

template <typename T>
void quick_sort(T arr[], int len)
{
    quick_sort_recursive(arr, 0, len - 1);
}

/* 调用 */
void test_quick_sort()
{
    int num_arr[] = {5, 3, 7, 6, 4, 1, 0, 2, 9, 10, 8};
    int length = sizeof(num_arr) / sizeof(int);
    quick_sort(num_arr, length);
    std::cout << "sorted result is ";
    for (int i = 0; i < length; i++)
    {
        std::cout << " " << num_arr[i];
    }

    std::cout << std::endl;
}

/* 更通用的方式，利用函数式编程思想 */
template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty()) /* input 为待排的序列 */
    {
        return input;
    }
    std::list<T> result; /* 排序完成存放结果的序列 */

    //  ① 将input中的第一个元素放入result中，并且将这第一个元素从input中删除
    result.splice(result.begin(), input, input.begin()); /* result 中的第一个元素就是基准元素 */

    //  ② 取result的第一个元素，将来用这个元素做切割，切割input中的列表。
    T const &pivot = *result.begin();

    //  ③std::partition 是一个标准库函数，用于将容器或数组中的元素按照指定的条件进行分区，
    // 使得满足条件的元素排在不满足条件的元素之前。
    // 所以经过计算divide_point指向的是input中第一个大于等于pivot的元素
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](T const &t)
                                       { return t < pivot; });

    // ④ 我们将小于pivot的元素放入lower_part中
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
                      divide_point);

    // ⑤我们将lower_part传递给sequential_quick_sort 返回一个新的有序的从小到大的序列
    // lower_part 中都是小于divide_point的值
    auto new_lower(
        sequential_quick_sort(std::move(lower_part)));
    // ⑥我们剩余的input列表传递给sequential_quick_sort递归调用，input中都是大于divide_point的值。
    auto new_higher(
        sequential_quick_sort(std::move(input)));
    // ⑦到此时new_higher和new_lower都是从小到大排序好的列表
    // 将new_higher 拼接到result的尾部
    result.splice(result.end(), new_higher);
    // 将new_lower 拼接到result的头部
    result.splice(result.begin(), new_lower);
    return result;
}

/* 调用 */
void test_sequential_quick()
{
    std::list<int> numlists = {6, 1, 0, 7, 5, 2, 9, -1};
    auto sort_result = sequential_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}

/* 并行方式 */
template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const &pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](T const &t)
                                       { return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
                      divide_point);
    // ①因为lower_part是副本，所以并行操作不会引发逻辑错误，这里可以启动future做排序
    std::future<std::list<T>> new_lower(
        std::async(&parallel_quick_sort<T>, std::move(lower_part)));
    // async 相当于开启了一个线程，并且这个线程的生命周期不需要我们去管，而是操作系统管理，我们不能控制线程的开启和关闭
    // 第一个基准元素排序完成之后，左边和右边排序其实互不干扰，可以采用并行的方式，同时进行排序
    // ②
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

/* 调用 */
void test_parallel_quick_sort()
{
    std::list<int> numlists = {6, 1, 0, 7, 5, 2, 9, -1};
    auto sort_result = parallel_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}

/* 线程池版本 */
// 线程池版本
// 并行版本
template <typename T>
std::list<T> thread_pool_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const &pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](T const &t)
                                       { return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
                      divide_point);
    // ①因为lower_part是副本，所以并行操作不会引发逻辑错误，这里投递给线程池处理
    auto new_lower = ThreadPool::commit(&parallel_quick_sort<T>, std::move(lower_part));
    // ②
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

/* 调用 */
void test_thread_pool_sort()
{
    std::list<int> numlists = {6, 1, 0, 7, 5, 2, 9, -1};
    auto sort_result = thread_pool_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}