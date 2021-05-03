#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque {
    public:
        explicit BlockDeque(size_t maxCapacity = 1000);

        ~BlockDeque();

        void close();

        void clear();

        bool empty();

        bool full();

        size_t size();

        size_t capacity();

        T front();

        T back();

        void push_front(const T &item);

        void push_back(const T &item);

        bool pop(T &item);

        bool pop(T &item, int timeout);

        void flush();

    private:
        std::deque<T> m_deq;

        size_t m_capacity;

        std::mutex m_mtx;

        bool m_isClose;

        std::condition_variable m_condConsumer;

        std::condition_variable m_condProducer;
};


template<class T>
BlockDeque<T>::BlockDeque(size_t maxCapacity): m_capacity(maxCapacity) {
    assert(maxCapacity > 0);
    m_isClose = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    close();
};

template<class T>
void BlockDeque<T>::close() {
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_deq.clear();
        m_isClose = true;
    }
    m_condProducer.notify_all();
    m_condConsumer.notify_all();
};

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(m_mtx);
    m_deq.clear();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.empty();
}

template<class T>
bool BlockDeque<T>::full() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.size() >= m_capacity;
}


template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_capacity;
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.back();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_deq.size() >= m_capacity) {
        m_condProducer.wait(locker);
    }
    m_deq.push_front(item);
    m_condConsumer.notify_one();
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_deq.size() >= m_capacity) {
        m_condProducer.wait(locker);
    }
    m_deq.push_back(item);
    m_condConsumer.notify_one();
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_deq.empty()) {
        m_condConsumer.wait(locker);
        if (m_isClose) {
            return false;
        }
    }
    item = m_deq.front();
    m_deq.pop_front();
    m_condProducer.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_deq.empty()) {
        if (m_condConsumer.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout) {
            return false;
        }
        if (m_isClose) {
            return false;
        }
    }
    item = m_deq.front();
    m_deq.pop_front();
    m_condProducer.notify_one();
    return true;
}

template<class T>
void BlockDeque<T>::flush() {
    m_condConsumer.notify_one();
};


#endif // BLOCKQUEUE_H