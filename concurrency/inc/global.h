#include <iostream>

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

    int GetData() const
    {
        return _count;
    }

private:
    int _count;
};