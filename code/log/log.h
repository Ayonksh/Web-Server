#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>

#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
    public:
        void init(int level, const char* path = "./log", 
                    const char* suffix =".log",
                    int maxQueueCapacity = 1024);

        static Log* getInstance();
        static void flushLogThread();

        void write(int level, const char *format,...);
        void flush();

        int getLevel();
        void setLevel(int level);
        bool isOpen() { return m_isOpen; }
        
    private:
        Log();
        virtual ~Log();
        void m_appendLogLevelTitle(int level);
        void m_asyncWrite();

    private:
        static const int LOG_PATH_LEN = 256;
        static const int LOG_NAME_LEN = 256;
        static const int MAX_LINES = 50000;

        const char* m_path;
        const char* m_suffix;

        int m_MAX_LINES;

        int m_lineCount;
        int m_toDay;

        bool m_isOpen;
    
        Buffer m_buff;
        int m_level;
        bool m_isAsync;

        FILE* m_fp;
        std::unique_ptr<BlockDeque<std::string>> m_deque; 
        std::unique_ptr<std::thread> m_writeThread;
        std::mutex m_mtx;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::getInstance();\
        if (log->isOpen() && log->getLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H