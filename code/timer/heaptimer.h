#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>

#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

class HeapTimer {
    public:
        HeapTimer() { m_heap.reserve(64); }

        ~HeapTimer() { clear(); }

        void adjust(int id, int newExpires);

        void add(int id, int timeout, const TimeoutCallBack& cb);

        void doWork(int id);

        void clear();

        void tick();

        void pop();

        int getNextTick();

    private:
        void m_del(size_t i);

        void m_siftUp(size_t i);

        bool m_siftDown(size_t index, size_t n);

        void m_swapNode(size_t i, size_t j);

        std::vector<TimerNode> m_heap;

        std::unordered_map<int, size_t> m_ref;
};

#endif  //HEAP_TIMER_H