#pragma once

#include <cstdint>
#include <klib/builtin.h>
#include <mrk/log.h>
#include <mrk/vmm.h>

#define _DEFAULT_QUEUE_SIZE 10 // Default size of a queue

namespace klib {
template <typename T>
class queue {
public:
    queue(uint32_t size = _DEFAULT_QUEUE_SIZE)
        : front(0)
        , rear(0)
        , _count(-1)
    {
        _array = new T[size];
        _memset((uint64_t)_array, 0, (sizeof(T) * size)); // Clear Buffer
        _capacity = size;
    }

    T pop();
    void push(T item);

    uint32_t size() { return _count; }
    bool is_empty() { return (_count == 0); }
    bool is_full() { return (_count == _capacity); }

private:
    void grow_buffer();
    T* _array;
    uint32_t _capacity;
    uint32_t front, rear, _count;
};

template <typename T>
void queue<T>::grow_buffer()
{
    void* oldptr = (void*)_array;
    void* newptr = new T[_capacity++];
    _memset((uint64_t)_array, 0, (sizeof(T) + _capacity));
    _memcpy(oldptr, newptr, _capacity - 1);

    _array = (T*)newptr;
}

template <typename T>
T queue<T>::pop()
{
    // Check for underflow
    if (is_empty()) {
        log("klib: Queue Underflow!\n");
        asm volatile("int $3"); // Trigger Exception
    }

    T to_return = _array[front];

    front = (front + 1) % _capacity;
    _count--;

    return to_return;
}

template <typename T>
void queue<T>::push(T item)
{
    // check for queue overflow
    if (is_full()) {
        // log("klib: Queue Overflow!\n");
        // asm volatile ("int $3");
        grow_buffer();
    }

    rear = (rear + 1) % _capacity;
    _array[rear] = item;
    _count++;
}
}
