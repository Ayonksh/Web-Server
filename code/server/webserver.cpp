#include "webserver.h"

using namespace std;

WebServer::WebServer(
            int port, int trigMode, int timeoutMS, bool optLinger,
            int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName,
            int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize):
            m_port(port), m_timeoutMS(timeoutMS), m_openLinger(optLinger), m_isClose(false),
            m_timer(new HeapTimer()), m_threadpool(new ThreadPool(threadNum)), m_epoller(new Epoller())
    {
    m_srcDir = getcwd(nullptr, 256);
    assert(m_srcDir);
    strncat(m_srcDir, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = m_srcDir;
    SqlConnPool::getInstance()->initPool("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    
    m_initEventMode(trigMode);
    if (!m_initSocket()) { m_isClose = true;}

    if (openLog) {
        Log::getInstance()->init(logLevel, "./log", ".log", logQueSize);
        if (m_isClose) {
            LOG_ERROR("========== Server init error!==========");
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", m_port, optLinger ? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (m_listenEvent & EPOLLET ? "ET": "LT"),
                            (m_connEvent & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer() {
    close(m_listenFd);
    m_isClose = true;
    free(m_srcDir);
    SqlConnPool::getInstance()->closePool();
}

void WebServer::start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if (!m_isClose) { LOG_INFO("========== Server start =========="); }
    while (!m_isClose) {
        if (m_timeoutMS > 0) {
            timeMS = m_timer->getNextTick();
        }
        /* 调用epoll_wait等待文件描述符上的事件，并将当前所有就绪的epoll_event复制到m_events数组中 */
        int eventCnt = m_epoller->wait(timeMS);
        for (int i = 0; i < eventCnt; ++i) {
            /* 处理事件 */
            int fd = m_epoller->getEventFd(i);
            uint32_t events = m_epoller->getEvents(i);
            i f(fd == m_listenFd) {
                m_dealListen();
            }
            /* 如有异常，则关闭客户连接 */
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(m_users.count(fd) > 0);
                m_closeConn(&m_users[fd]);
            } else if (events & EPOLLIN) {
                assert(m_users.count(fd) > 0);
                m_dealRead(&m_users[fd]);
            } else if (events & EPOLLOUT) {
                assert(m_users.count(fd) > 0);
                m_dealWrite(&m_users[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::m_initEventMode(int trigMode) {
    m_listenEvent = EPOLLRDHUP;
    m_connEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode) {
        case 0:
            break;
        case 1:
            m_connEvent |= EPOLLET;
            break;
        case 2:
            m_listenEvent |= EPOLLET;
            break;
        case 3:
            m_listenEvent |= EPOLLET;
            m_connEvent |= EPOLLET;
            break;
        default:
            m_listenEvent |= EPOLLET;
            m_connEvent |= EPOLLET;
            break;
    }
    HttpConn::isET = (m_connEvent & EPOLLET);
}

/* Create listenFd */
bool WebServer::m_initSocket() {
    if (m_port > 65535 || m_port < 1024) {
        LOG_ERROR("Port:%d error!",  m_port);
        return false;
    }
    int ret;
    /* 创建监听socket的TCP/IP的一个IPv4 socket地址 */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY：将套接字绑定到所有可用的接口
    addr.sin_port = htons(m_port);
    
    struct linger optLinger = { 0 };
    if (m_openLinger) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);  //PF_INET用于IPv4，SOCK_STREAM表示传输层使用TCP协议，在前两个参数都设置好的情况下，第三个参数设置为0，表示使用默认协议
    if (m_listenFd < 0) {
        LOG_ERROR("Create socket error!", m_port);
        return false;
    }

    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));  //SO_LINGER选项根据不同的linger结构体成员变量值来控制close系统调用在关闭TCP连接时的行为
    if (ret < 0) {
        close(m_listenFd);
        LOG_ERROR("Init linger error!", m_port);
        return false;
    }
    
    int reuse = 1;
    /* 端口复用 */
    /* 但是，这些套接字并不是所有都能读取信息，只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, sizeof(int));  //reuse=0，将已经处于连接状态的socket在调用close(socket)后强制关闭，不经历TIME_WAIT的过程
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(m_listenFd);
        return false;
    }

    /* 绑定socket文件描述符和监听socket的TCP/IP的IPV4 socket地址 */
    ret = bind(m_listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", m_port);
        close(m_listenFd);
        return false;
    }

    /* 创建监听队列以存放待处理的客户连接，在这些客户连接被accept()之前 */
    ret = listen(m_listenFd, 5);    ////backlog参数表示内核监听队列的最大长度，典型值是5
    if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", m_port);
        close(m_listenFd);
        return false;
    }

    /* 将m_listenFd以 EPOLLIN 和 m_listenEvent 为事件类型，注册到m_epoller中m_epollFd指示的epoll内核事件中 */
    /* 当listen到新的客户连接时，m_listenFd变为就绪事件 */
    ret = m_epoller->addFd(m_listenFd, m_listenEvent | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(m_listenFd);
        return false;
    }
    
    setFdNonblock(m_listenFd);
    LOG_INFO("Server port:%d", m_port);
    return true;
}

void WebServer::m_addClient(int connFd, sockaddr_in addr) {
    assert(connFd > 0);
    m_users[connFd].init(connFd, addr);
    if (m_timeoutMS > 0) {
        m_timer->add(connFd, m_timeoutMS, std::bind(&WebServer::m_closeConn, this, &m_users[connFd]));
    }
    m_epoller->addFd(connFd, EPOLLIN | m_connEvent);
    setFdNonblock(connFd);
    LOG_INFO("Client[%d] in!", m_users[connFd].getFd());
}

void WebServer::m_dealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int connFd = accept(m_listenFd, (struct sockaddr *)&addr, &len);
        if (connFd <= 0) { return; }
        else if (HttpConn::userCount >= MAX_FD) {
            m_sendError(connFd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        m_addClient(connFd, addr);
    } while (m_listenEvent & EPOLLET);
}

void WebServer::m_dealRead(HttpConn* client) {
    assert(client);
    m_extentTime(client);
    m_threadpool->addTask(std::bind(&WebServer::m_onRead, this, client));
}

void WebServer::m_dealWrite(HttpConn* client) {
    assert(client);
    m_extentTime(client);
    m_threadpool->addTask(std::bind(&WebServer::m_onWrite, this, client));
}

void WebServer::m_sendError(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::m_extentTime(HttpConn* client) {
    assert(client);
    if (m_timeoutMS > 0) { m_timer->adjust(client->getFd(), m_timeoutMS); }
}

void WebServer::m_closeConn(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    m_epoller->delFd(client->getFd());
    client->closeConn();
}

void WebServer::m_onRead(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        m_closeConn(client);
        return;
    }
    m_onProcess(client);
}

void WebServer::m_onProcess(HttpConn* client) {
    if (client->process()) {
        m_epoller->modFd(client->getFd(), m_connEvent | EPOLLOUT);
    } else {
        m_epoller->modFd(client->getFd(), m_connEvent | EPOLLIN);
    }
}
 
void WebServer::m_onWrite(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->toWriteBytes() == 0) {
        /* 传输完成 */
        if (client->isKeepAlive()) {
            m_onProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            /* 继续传输 */
            m_epoller->modFd(client->getFd(), m_connEvent | EPOLLOUT);
            return;
        }
    }
    m_closeConn(client);
}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);    //用F_GETFL获取文件描述符旧的状态标志位，再用F_SETFL设置为新的非阻塞的状态标志位
}
