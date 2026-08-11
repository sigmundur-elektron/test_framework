#pragma once
template<typename T>
class singleton {
public:
    static T& get_instance();

    singleton(const singleton&) = delete;
    singleton& operator= (const singleton) = delete;

protected:
    struct token {};
    singleton() {}
};

#include <memory>
template<typename T>
T& singleton<T>::get_instance()
{
    static T instance{token{}};
    return instance;
}