#pragma once

template<typename T>
class Singleton
{
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static T& GetInstnace()
    {
        if(instance)
        {
            return *instance;
        }
        instance = new T();
        return *instance;
    }

protected:
    Singleton()
    {
    }

    ~Singleton()
    {
        if(instance)
        {
            delete instance;
        }
    }

protected:
    static T* instance;
};

template <typename T>
T* Singleton<T>::instance = nullptr;
