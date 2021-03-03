#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
    public:
        HttpConn();
        ~HttpConn();

        void init(int sockFd, const sockaddr_in& addr);
        void reset();
        ssize_t read(int* saveErrno);
        ssize_t write(int* saveErrno);

        void closeConn();

        int getFd() const;
        int getPort() const;
        const char* getIP() const;
        sockaddr_in getAddr() const;
        
        void process();
        int toWriteBytes() { 
            return m_iov[0].iov_len + m_iov[1].iov_len; 
        }
        bool isKeepAlive() const {
            return m_request.isKeepAlive();
        }

        static bool isET;
        static const char* srcDir;
        static std::atomic<int> userCount;
        
    private:
    
        int m_fd;
        struct  sockaddr_in m_addr;

        bool m_isClose;
        
        int m_iovCnt;
        struct iovec m_iov[2];
        
        Buffer m_readBuff; // 读缓冲区
        Buffer m_writeBuff; // 写缓冲区

        HttpRequest m_request;
        HttpResponse m_response;
};


#endif //HTTP_CONN_H