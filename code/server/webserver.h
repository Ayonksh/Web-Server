#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
    public:
        WebServer(
            int port, int trigMode, int timeoutMS, bool optLinger, 
            int sqlPort, const char* sqlUser, const  char* sqlPwd, const char* dbName,
            int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize);

        ~WebServer();
        void start();

    private:
        void m_initEventMode(int trigMode);
        bool m_initSocket(); 
        void m_addClient(int fd, sockaddr_in addr);
    
        void m_dealListen();
        void m_dealWrite(HttpConn* client);
        void m_dealRead(HttpConn* client);

        void m_sendError(int fd, const char*info);
        void m_extentTime(HttpConn* client);
        void m_closeConn(HttpConn* client);

        void m_onRead(HttpConn* client);
        void m_onWrite(HttpConn* client);
        
        static const int MAX_FD = 65536;

        static int setFdNonblock(int fd);

        int m_port;
        int m_timeoutMS;  /* 毫秒MS */
        bool m_openLinger;
        bool m_isClose;
        int m_listenFd;
        char* m_srcDir;
        
        uint32_t m_listenEvent;
        uint32_t m_connEvent;
    
        std::unique_ptr<HeapTimer> m_timer;
        std::unique_ptr<ThreadPool> m_threadpool;
        std::unique_ptr<Epoller> m_epoller;
        std::unordered_map<int, HttpConn> m_users;
};


#endif //WEBSERVER_H