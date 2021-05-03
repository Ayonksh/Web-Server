#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <assert.h>

class Buffer {
    public:
        Buffer(int initBuffSize = 1024);
        ~Buffer() = default;

        size_t readableBytes() const;
        size_t writableBytes() const;
        size_t prependableBytes() const;

        const char* peek() const;
        void ensureWriteable(size_t len);
        void hasWritten(size_t len);

        void retrieve(size_t len);
        void retrieveUntil(const char* end);
        void retrieveAll();
        std::string retrieveAllToStr();

        const char* beginWriteConst() const;
        char* beginWrite();

        void append(const std::string& str);
        void append(const char* str, size_t len);
        void append(const void* data, size_t len);
        void append(const Buffer& buff);

        ssize_t readFd(int fd, int* Errno);
        ssize_t writeFd(int fd, int* Errno);

    private:
        char* m_beginPtr();
        const char* m_beginPtr() const;
        void m_makeSpace(size_t len);

        std::vector<char> m_buffer;
        std::atomic<std::size_t> m_readPos;
        std::atomic<std::size_t> m_writePos;
};

#endif  //BUFFER_H