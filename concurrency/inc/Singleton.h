//
// Created by mater on 2024/5/17.
//

#include <memory>
#include <mutex>
#include <iostream>

template <typename T>
class Singleton
{
protected:
    Singleton() = default;
    Singleton(const Singleton<T> &) = delete;
    Singleton &operator=(const Singleton<T> &) = delete;
    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> getInstance()
    {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]()
                       { _instance = std::shared_ptr<T>(new T); });
        return _instance;
    }

    void printAddress()
    {
        std::cout << _instance.get() << std::endl; /* 获得智能指针对应的裸指针 */
    }

    ~Singleton()
    {
        std::cout << "this is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

/* 静态成员变量应该在 .cpp 文件中初始化 */
/* template <typename T>
   std::shared_ptr<T> Singleton<T>::_instance = nullptr;  */
