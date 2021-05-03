#include "sqlconnpool.h"

using namespace std;

SqlConnPool::SqlConnPool() {
    m_curConn = 0;
    m_freeConn = 0;
}

SqlConnPool* SqlConnPool::getInstance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::initPool(const char* host, int port,
            const char* username, const char* password,
            const char* dbName, int connSize = 10) {

    for (int i = 0; i < connSize; i++) {
        MYSQL *sqlConn = nullptr;
        sqlConn = mysql_init(sqlConn);

        if (!sqlConn) {
            LOG_ERROR("Mysql Init Error!");
            exit(1);
        }

        sqlConn = mysql_real_connect(sqlConn, host, username, password, dbName, port, nullptr, 0);
        if (!sqlConn) {
            LOG_ERROR("Mysql Connect Error!");
            exit(1);
        }

        m_connQueue.push(sqlConn);
        m_freeConn++;
    }
    m_maxConn = connSize;
    sem_init(&m_semId, 0, m_maxConn);  //sem_init()第二个参数为0表示该信号量是当前进程的局部信号量，第三个参数表示信号量的初始值
}

MYSQL* SqlConnPool::getConn() {
    MYSQL *sqlConn = nullptr;
    if (m_connQueue.empty()){
        LOG_WARN("SqlConnPool Busy!");
        return nullptr;
    }

    sem_wait(&m_semId);
    {
        lock_guard<mutex> locker(m_mutex);
        sqlConn = m_connQueue.front();
        m_connQueue.pop();

        m_freeConn--;
        m_curConn++;
    }
    return sqlConn;
}

void SqlConnPool::freeConn(MYSQL* sqlConn) {
    assert(sqlConn);
    lock_guard<mutex> locker(m_mutex);
    m_connQueue.push(sqlConn);
    m_freeConn++;
    m_curConn--;
    sem_post(&m_semId);
}

int SqlConnPool::getFreeConnCount() {
    lock_guard<mutex> locker(m_mutex);
    return m_freeConn;  // return m_connQueue.size();
}

void SqlConnPool::closePool() {
    lock_guard<mutex> locker(m_mutex);
    while (!m_connQueue.empty()) {
        auto item = m_connQueue.front();
        m_connQueue.pop();
        mysql_close(item);
    }
    m_freeConn = 0;
    m_curConn = 0;
    mysql_library_end();
}

SqlConnPool::~SqlConnPool() {
    closePool();
}
