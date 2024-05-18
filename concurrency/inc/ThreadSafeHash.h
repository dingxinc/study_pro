#include <iostream>
#include <set>
#include <thread>
#include <vector>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <iterator>
#include <map>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table
{
private:
    // 桶类型
    class bucket_type
    {
        friend class threadsafe_lookup_table;

    private:
        // 存储元素的类型为pair，由key和value构成
        typedef std::pair<Key, Value> bucket_value;
        // 由链表存储元素构
        typedef std::list<bucket_value> bucket_data;
        // 链表的迭代器
        typedef typename bucket_data::iterator bucket_iterator;
        // 链表数据
        bucket_data data;
        // 改用共享锁
        mutable std::shared_mutex mutex;
        // 查找操作，在list中找到匹配的key值，然后返回迭代器
        bucket_iterator find_entry_for(const Key &key)
        {
            return std::find_if(data.begin(), data.end(),
                                [&](bucket_value const &item)
                                { return item.first == key; });
        }

    public:
        // 查找key值，找到返回对应的value，未找到则返回默认值
        Value value_for(Key const &key, Value const &default_value)
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            return (found_entry == data.end()) ? default_value : found_entry->second;
        }
        // 添加key和value，找到则更新，没找到则添加
        void add_or_update_mapping(Key const &key, Value const &value)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if (found_entry == data.end())
            {
                data.push_back(bucket_value(key, value));
            }
            else
            {
                found_entry->second = value;
            }
        }
        // 删除对应的key
        void remove_mapping(Key const &key)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if (found_entry != data.end())
            {
                data.erase(found_entry);
            }
        }
    };
    // 用vector存储桶类型
    std::vector<std::unique_ptr<bucket_type>> buckets;
    // hash<Key> 哈希表 用来根据key生成哈希值
    Hash hasher;

    // 根据key生成数字，并对桶的大小取余得到下标，根据下标返回对应的桶智能指针
    bucket_type &get_bucket(Key const &key) const
    {
        std::size_t const bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }

public:
    threadsafe_lookup_table(
        unsigned num_buckets = 19, Hash const &hasher_ = Hash()) : buckets(num_buckets), hasher(hasher_)
    {
        for (unsigned i = 0; i < num_buckets; ++i)
        {
            buckets[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const &other) = delete;
    threadsafe_lookup_table &operator=(
        threadsafe_lookup_table const &other) = delete;

    Value value_for(Key const &key,
                    Value const &default_value = Value())
    {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(Key const &key, Value const &value)
    {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(Key const &key)
    {
        get_bucket(key).remove_mapping(key);
    }

    std::map<Key, Value> get_map()
    {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (unsigned i = 0; i < buckets.size(); ++i)
        {
            locks.push_back(
                std::unique_lock<std::shared_mutex>(buckets[i]->mutex));
        }
        std::map<Key, Value> res;
        for (unsigned i = 0; i < buckets.size(); ++i)
        {
            // 需用typename告诉编译器bucket_type::bucket_iterator是一个类型，以后再实例化
            // 当然此处可简写成auto it = buckets[i]->data.begin();
            typename bucket_type::bucket_iterator it = buckets[i]->data.begin();
            for (; it != buckets[i]->data.end(); ++it)
            {
                res.insert(*it);
            }
        }
        return res;
    }
};

/* 测试 */
class MyClass
{
public:
    MyClass() {}
    MyClass(int count) : _count(count) {}
    MyClass(const MyClass &mc) : _count(mc._count) {}
    MyClass(MyClass &&mc) : _count(mc._count) {}

    friend std::ostream &operator<<(std::ostream &os, const MyClass &mc)
    {
        os << mc._count;
        return os;
    }

private:
    int _count;
};

void TestThreadSafeHash()
{
    std::set<int> removeSet;
    threadsafe_lookup_table<int, std::shared_ptr<MyClass>> table;
    std::thread t1([&]()
                   {
    	for(int i = 0; i < 100; i++)
    	{
           auto class_ptr =  std::make_shared<MyClass>(i); 
            table.add_or_update_mapping(i, class_ptr);
    	} });

    std::thread t2([&]()
                   {
        for (int i = 0; i < 100; )
        {
            auto find_res = table.value_for(i, nullptr);
            if(find_res)
            {
                table.remove_mapping(i);
                removeSet.insert(i);
                i++;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } });

    std::thread t3([&]()
                   {
        for (int i = 100; i < 200; i++)
        {
            auto class_ptr = std::make_shared<MyClass>(i);
            table.add_or_update_mapping(i, class_ptr);
        } });

    t1.join();
    t2.join();
    t3.join();

    for (auto &i : removeSet)
    {
        std::cout << "remove data is " << i << std::endl;
    }

    auto copy_map = table.get_map();
    for (auto &i : copy_map)
    {
        std::cout << "copy data is " << *(i.second) << std::endl;
    }
}