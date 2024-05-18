//
// Created by mater on 2024/5/9.
//

#ifndef STL_VECTOR__H
#define STL_VECTOR__H

#include <string>

template <typename T>
class vector_
{
public:
    vector_() : elements(nullptr), first_free(nullptr), tail(nullptr) {}
    // 拷贝构造函数
    vector_(const vector_ &);
    // 拷贝赋值运算符
    vector_ &operator=(const vector_ &);
    // 移动构造函数
    vector_(vector_ &&src) noexcept : elements(src.elements), first_free(src.first_free), tail(src.tail)
    {
        // 将源数据置空
        src.elements = src.first_free = src.tail = nullptr;
    }

    template <typename... Args>
    void emplace_back(Args &&...args);

    // 析构函数
    ~vector_();
    // 插入元素
    void push_back(const T &);
    // 抛出元素
    void pop_back(T &t);
    // 返回元素个数
    size_t size() const { return first_free - elements; }
    // 返回数组容量
    size_t capacity() const { return tail - elements; }
    // 返回首元素的指针
    T *begin() const { return elements; }
    // 返回最后一个元素地址（第一个空闲元素地址）
    T *end() const { return first_free; }

private:
    // 判断容量不足，开辟新空间
    void check_n_alloc()
    {
        if (size() == capacity())
        {
            // 数组容量不足，需要扩容
            reallocate();
        }
    }

    // 扩容函数
    void reallocate();
    // 复制元素到新空间
    std::pair<T *, T *> copy_n_elements(const T *, const T *);
    // 释放空间
    void free_memory();

private:
    // 数组首元素的指针
    T *elements;
    // 数组第一个空闲元素的指针
    T *first_free;
    // 数组尾后位置的指针，最后一个位置 + 1，其实是个无效位置
    T *tail;
    // 初始化 alloc 用来分配空间
    static std::allocator<T> alloc;
};

/* Entry implementation ------------------------------------------------------------ */

template <typename T>
std::allocator<T> vector_<T>::alloc;

// 实现区间copy
template <typename T>
std::pair<T *, T *> vector_<T>::copy_n_elements(const T *first, const T *last)
{
    auto newdata = alloc.allocate(last - first);
    return {newdata, std::uninitialized_copy(first, last, newdata)};
}

// 实现拷贝构造函数
template <typename T>
vector_<T>::vector_(const vector_ &src)
{
    // 先判断是否为空
    auto rsp = copy_n_elements(src.begin(), src.end());
    elements = rsp.first;
    first_free = rsp.second;
    tail = elements + src.size();
}

// 实现拷贝赋值运算符
template <typename T>
vector_<T> &vector_<T>::operator=(const vector_ &src)
{
    if (this != &src)
    {
        auto rsp = copy_n_elements(src.begin(), src.end());
        elements = rsp.first;
        first_free = rsp.second;
        tail = rsp.second;
    }
    return *this;
}

// 实现析构函数
template <typename T>
vector_<T>::~vector_()
{
    // 先判断是否为空
    if (elements == nullptr)
        return;
    // 缓存第一个元素
    auto dest = elements;
    // 循环析构
    for (size_t i = 0; i < size(); ++i)
    {
        alloc.destroy(dest++);
    }
    // 回收内存
    alloc.deallocate(elements, tail - elements); // tail - elements 就是数组容量, tail 是第一个无效元素， elements 是第一个有效元素
    elements = nullptr;
    first_free = nullptr;
    tail = nullptr;
}

// 实现扩容函数
template <typename T>
void vector_<T>::reallocate()
{
    T *newdate = nullptr;
    // 数组为空的情况，构造一个空间
    if (elements == nullptr || first_free == nullptr || tail == nullptr)
    {
        newdata = alloc.allocate(1);
        elements = newdata;
        first_free = newdata;
        tail = newdata + 1;
        return;
    }
    // 原数据不为空，则扩容 size() 两倍大小
    newdata = alloc.allocate(size() * 2);
    // 新空间的空闲位置
    auto dest = newdata;
    // 旧空间的有效位置
    auto src = elements;
    // 循环拷贝
    for (size_t i = 0; i != size(); ++i)
    {
        alloc.construct(dest++, std::move(*src++)); // 一边拷贝一边构造
    }
    // 释放旧空间
    free_memory();
    // 更新数据
    elements = newdata;
    first_free = dest; // dest 是一直 ++ 的
    tail = newdata + size() * 2;
}

// 实现 push_back 函数
template <typename T>
void vector_<T>::push_back(const T &t)
{
    check_n_alloc(); // 先检测容量是否足够
    alloc.construct(first_free++, t);
}

// 实现 pop_back 函数
template <typename T>
void vector_<T>::pop_back(T &t)
{
    // 先判断是否为空
    if (first_free == nullptr)
        return;
    // 判断 size 为 1
    if (size() == 1)
    {
        t = std::move(*elements);
        alloc.destroy(elemets);
        first_free = nullptr;
        elements = nullptr;
        return;
    }
    // 缓存最后一个元素
    t = std::move(*--first_free);
    alloc.destroy(first_free);
}

template <typename T>
void vector_<T>::free_memory()
{
    if (elements == nullptr)
        return;
    auto dest = elements;
    for (size_t i = 0; i < size(); ++i)
    {
        alloc.destroy(dest++);
    }
    // 再整体回收内存
    alloc.deallocate(elements, tail - elements);
    elements = nullptr;
    first_free = nullptr;
    tail = nullptr;
}

template <typename T>
template <typename... Args>
void vector_<T>::emplace_back(Args &&...args)
{
    alloc.construct(first_free++, std::forward<Args>(args)...); // 比如 emplace_back(10, 'c')，相当于 alloc.constuct(first_free++, std::forward<int>(10), std::forward<char>('c'))
}

#endif // STL_VECTOR__H
