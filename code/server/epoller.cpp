#include "epoller.h"

/* epoll_create 创建一个额外的文件描述符m_epollFd来唯一标识内核中的epoll事件表 */
/* m_events数组用于存储epoll事件表中的就绪事件 */
Epoller::Epoller(int maxEvent):m_epollFd(epoll_create(512)), m_events(maxEvent) {
    assert(m_epollFd >= 0 && m_events.size() > 0);
}

Epoller::~Epoller() {
    close(m_epollFd);
}

bool Epoller::addFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::modFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::delFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::wait(int timeoutMS) {
    return epoll_wait(m_epollFd, &m_events[0], static_cast<int>(m_events.size()), timeoutMS);
}

int Epoller::getEventFd(size_t i) const {
    assert(i < m_events.size() && i >= 0);
    return m_events[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const {
    assert(i < m_events.size() && i >= 0);
    return m_events[i].events;
}