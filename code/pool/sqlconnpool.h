#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
    public:
        SqlConnPool();
        ~SqlConnPool();

        static SqlConnPool *getInstance();

        MYSQL *getConn();               //获取连接
        void freeConn(MYSQL *sqlConn);  //释放连接
        int getFreeConnCount();

        void initPool(const char* host, int port,
                const char* username,const char* password, 
                const char* dbName, int connSize);
        void closePool();

    private:
        unsigned int m_maxConn;  //最大连接数
        unsigned int m_curConn;  //当前连接数
        unsigned int m_freeConn; //空闲连接数

        std::queue<MYSQL *> m_connQueue;
        std::mutex m_mutex;
        sem_t m_semId;
};

#endif  //SQLCONNPOOL_H