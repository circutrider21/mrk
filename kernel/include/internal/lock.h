#pragma once

#include <atomic>
#include <cstdint>

class spinlock {
    uint16_t _lock;

public:
    constexpr spinlock()
        : _lock(0)
    {
    }
    void lock();
    void unlock();
    bool try_lock();
};

class mutex {
    int status;

public:
    void lock();
    void unlock();
    bool try_lock();
};

template <typename Lock>
class lock_retainer {
    Lock& lk;

public:
    explicit lock_retainer(Lock& l)
        : lk(l)
    {
        lk.lock();
    }

    ~lock_retainer()
    {
        lk.unlock();
    }

    lock_retainer& operator=(const lock_retainer&) = delete;
    lock_retainer(const lock_retainer&) = delete;
};
