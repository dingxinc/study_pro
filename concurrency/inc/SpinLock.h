// 自旋锁

#include <iostream>
#include <mutex>
#include <thread>

/*
自旋锁是一种在多线程环境下保护共享资源的同步机制。它的基本思想是，当一个线程尝试获取锁时，
如果锁已经被其他线程持有，那么该线程就会不断地循环检查锁的状态，直到成功获取到锁为止。*/

class SpinLock
{
public:
    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire))
            ; // 自旋等待，知道成功获取锁
    }

    void unlock()
    {
        while (flag.test_and_set(std::memory_order_release))
            ; // 释放锁
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

/**
 * 1 处 在多线程调用时，仅有一个线程在同一时刻进入test_and_set，因为atomic_flag初始状态为false,所以test_and_set将atomic_flag设置为true，并且返回false。

比如线程A调用了test_and_set返回false，这样lock函数返回，线程A继续执行加锁区域的逻辑。此时线程B调用test_and_set，test_and_set会返回true,导致线程B在while循环中循环等待，
达到自旋检测标记的效果。当线程A执行至2处调用clear操作后，atomic_flag被设置为清空状态，线程B调用test_and_set会将状态设为成立并返回false，B线程执行加锁区域的逻辑。
 */