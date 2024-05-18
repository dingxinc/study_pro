#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <cassert>
#include <algorithm>
#include <memory>

/*
 *  memory_order_seq_cst代表全局一致性顺序，可以用于 store, load 和 read-modify-write 操作, 实现 sequencial consistent 的
 *  顺序模型. 在这个模型下, 所有线程看到的所有操作都有一个一致的顺序, 即使这些操作可能针对不同的变量, 运行在不同的线程.
 *  为了做到全局顺序一致, 每次写入操作都必须同步给其他核心，所以效率会有一定的下降，但是比锁的效率要高
 */

std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed); // 1
    y.store(true, std::memory_order_relaxed); // 2
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_relaxed))
    { // 3
        std::cout << "y load false" << std::endl;
    }

    if (x.load(std::memory_order_relaxed))
    { // 4
        ++z;
    }
}

void TestOrderRelaxed()
{

    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0); // 5  这个断言不会触发
}

/**
 * memory_order_relaxed 可以用于 store, load 和 read-modify-write 操作,
 *  实现 relaxed 的顺序模型. 只能保证操作的原子性和修改顺序 (modification order) 一致性, 无法实现 synchronizes-with 的关系。
 */
void TestOrderRelaxed()
{
    std::atomic<bool> rx, ry;

    std::thread t1([&]()
                   {
                       rx.store(true, std::memory_order_relaxed); // 1 // sequence-before
                       ry.store(true, std::memory_order_relaxed); // 2
                   });

    std::thread t2([&]()
                   {
                       while (!ry.load(std::memory_order_relaxed))
                           ;                                       // 3 当 ry 为 true 的时候才能退出 while 循环
                       assert(rx.load(std::memory_order_relaxed)); // 4
                   });

    t1.join();
    t2.join();
}

/**
 * acquire-release 模型:任何指令都不能重排到 acquire 操作的前面, 且不能重排到 release 操作的后面;
 *  因此很多需要实现 synchronizes-with 关系的场景都会使用 acquire-release.
 */
void TestReleaseAcquire()
{
    std::atomic<bool> rx, ry;

    std::thread t1([&]()
                   {
                       rx.store(true, std::memory_order_relaxed); // 1
                       ry.store(true, std::memory_order_release); // 2
                   });

    std::thread t2([&]()
                   {
                       while (!ry.load(std::memory_order_acquire))
                           ;                                       // 3   2-3 构成 acquire-release
                       assert(rx.load(std::memory_order_relaxed)); // 4  1 一定在 4 之前执行，断言一定不会触发
                   });

    t1.join();
    t2.join();
}

/**
 * release sequences模型：多个线程对同一个变量release操作，另一个线程对这个变量acquire，那么只有一个线程的release操作喝这个acquire线程构成同步关系
 */
void ReleasAcquireDanger2()
{
    std::atomic<int> xd{0}, yd{0};
    std::atomic<int> zd;

    std::thread t1([&]()
                   {
                       xd.store(1, std::memory_order_release); // (1)
                       yd.store(1, std::memory_order_release); //  (2)
                   });

    std::thread t2([&]()
                   {
                       yd.store(2, std::memory_order_release); // (3)
                   });

    std::thread t3([&]()
                   {
                       while (!yd.load(std::memory_order_acquire))
                           ;                                            // （4）  当 3-4 构成 acquire-release时，1可能并没有修改为 true ，5 会触发断言
                       assert(xd.load(std::memory_order_acquire) == 1); // (5)    当 2-4 构成 acquire-release ，则 1 一定执行了，此处断言不会触发
                   });

    t1.join();
    t2.join();
    t3.join();
}