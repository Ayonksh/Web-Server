#include "buffer.h"

Buffer::Buffer(int initBufferSize) : m_buffer(initBufferSize), m_readPos(0), m_writePos(0) {}

size_t Buffer::readableBytes() const {
    return m_writePos - m_readPos;
}

size_t Buffer::writableBytes() const {
    return m_buffer.size() - m_writePos;
}

size_t Buffer::prependableBytes() const {
    return m_readPos;
}

const char* Buffer::peek() const {
    return m_beginPtr() + m_readPos;
}

void Buffer::ensureWriteable(size_t len) {
    if(writableBytes() < len) {
        m_makeSpace(len);
    }
    assert(writableBytes() >= len);
}

void Buffer::hasWritten(size_t len) {
    m_writePos += len;
} 

void Buffer::retrieve(size_t len) {
    assert(len <= readableBytes());
    m_readPos += len;
}

void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

void Buffer::retrieveAll() {
    bzero(&m_buffer[0], m_buffer.size());
    m_readPos = 0;
    m_writePos = 0;
}

std::string Buffer::retrieveAllToStr() {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

const char* Buffer::beginWriteConst() const {
    return m_beginPtr() + m_writePos;
}

char* Buffer::beginWrite() {
    return m_beginPtr() + m_writePos;
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void Buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensureWriteable(len);
    std::copy(str, str + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const Buffer& buff) {
    append(buff.peek(), buff.readableBytes());
}

ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = writableBytes();
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = m_beginPtr() + m_writePos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        m_writePos += len;
    }
    else {
        m_writePos = m_buffer.size();
        append(buff, len - m_writePos);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    size_t readSize = readableBytes();
    ssize_t len = write(fd, peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    m_readPos += len;
    return len;
}

char* Buffer::m_beginPtr() {
    return &*m_buffer.begin();
}

const char* Buffer::m_beginPtr() const {
    return &*m_buffer.begin();
}

void Buffer::m_makeSpace(size_t len) {
    if(writableBytes() + prependableBytes() < len) {
        m_buffer.resize(m_writePos + len + 1);
    } 
    else {
        size_t readable = readableBytes();
        std::copy(m_beginPtr() + m_readPos, m_beginPtr() + m_writePos, m_beginPtr());
        m_readPos = 0;
        m_writePos = m_readPos + readable;
        assert(readable == readableBytes());
    }
}