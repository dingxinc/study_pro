// 写三个线程交替打印 ABC
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>

std::mutex mtx;
std::condition_variable cond;
int flag = 0;

void printa()
{
    std::unique_lock<std::mutex> lock(mtx);
    int count = 0;
    while (count < 10)
    {
        while (flag != 0)
            cond.wait(lock);
        std::cout << "thread 1: a" << std::endl;
        flag = 1;
        cond.notify_all();
        count++;
    }
}

void printb()
{
    std::unique_lock<std::mutex> lock(mtx);
    for (int i = 0; i < 10; ++i)
    {
        while (flag != 1)
            cond.wait(lock);
        std::cout << "thread 2: b" << std::endl;
        flag = 2;
        cond.notify_all();
    }
}

void printc()
{
    std::unique_lock<std::mutex> lock(mtx);
    for (int i = 0; i < 10; ++i)
    {
        while (flag != 2)
            cond.wait(lock);
        std::cout << "thread 3: c" << std::endl;
        flag = 0;
        cond.notify_all();
    }
}