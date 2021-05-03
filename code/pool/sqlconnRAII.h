#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

/* 资源在对象构造时初始化 资源在对象析构时释放*/
class SqlConnRAII {
    public:
        SqlConnRAII(MYSQL** sqlConn, SqlConnPool *connPool) {
            assert(connPool);
            *sqlConn = connPool->getConn();
            m_ConnRAII = *sqlConn;
            m_ConnPoolRAII = connPool;
        }
        
        ~SqlConnRAII() {
            if(m_ConnRAII) { m_ConnPoolRAII->freeConn(m_ConnRAII); }
        }

    private:
        MYSQL *m_ConnRAII;
        SqlConnPool* m_ConnPoolRAII;
};

#endif  //SQLCONNRAII_H