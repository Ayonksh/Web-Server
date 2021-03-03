#include "log.h"

using namespace std;

Log::Log() {
    m_lineCount = 0;
    m_isAsync = false;
    m_writeThread = nullptr;
    m_deque = nullptr;
    m_toDay = 0;
    m_fp = nullptr;
}

Log::~Log() {
    if(m_writeThread && m_writeThread->joinable()) {
        while(!m_deque->empty()) {
            m_deque->flush();
        };
        m_deque->close();
        m_writeThread->join();
    }
    if(m_fp) {
        lock_guard<mutex> locker(m_mtx);
        flush();
        fclose(m_fp);
    }
}

Log* Log::getInstance() {
    static Log serverLog;
    return &serverLog;
}

void Log::init(int level = 1, const char* path, const char* suffix, int maxQueueSize) {
    m_isOpen = true;
    m_level = level;
    if(maxQueueSize > 0) {
        m_isAsync = true;
        if(!m_deque) {
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            m_deque = move(newDeque);
            
            std::unique_ptr<std::thread> NewThread(new thread(flushLogThread));
            m_writeThread = move(NewThread);
        }
    } else {
        m_isAsync = false;
    }

    m_lineCount = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    m_path = path;
    m_suffix = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            m_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, m_suffix);
    m_toDay = t.tm_mday;

    {
        lock_guard<mutex> locker(m_mtx);
        m_buff.retrieveAll();
        if(m_fp) { 
            fflush(m_fp);
            fclose(m_fp); 
        }

        m_fp = fopen(fileName, "a");
        if(m_fp == nullptr) {
            mkdir(m_path, 0777);
            m_fp = fopen(fileName, "a");
        } 
        assert(m_fp != nullptr);
    }
}

void Log::flushLogThread() {
    Log::getInstance()->m_asyncWrite();
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (m_toDay != t.tm_mday || (m_lineCount && (m_lineCount  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(m_mtx);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (m_toDay != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", m_path, tail, m_suffix);
            m_toDay = t.tm_mday;
            m_lineCount = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, (m_lineCount  / MAX_LINES), m_suffix);
        }
        
        locker.lock();
        fflush(m_fp);
        fclose(m_fp);
        m_fp = fopen(newFile, "a");
        assert(m_fp != nullptr);
    }

    {
        unique_lock<mutex> locker(m_mtx);
        m_lineCount++;
        int n = snprintf(m_buff.beginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        m_buff.hasWritten(n);
        m_appendLogLevelTitle(level);

        va_start(vaList, format);
        int m = vsnprintf(m_buff.beginWrite(), m_buff.writableBytes(), format, vaList);
        va_end(vaList);

        m_buff.hasWritten(m);
        m_buff.append("\n\0", 2);

        if(m_isAsync && m_deque && !m_deque->full()) {
            m_deque->push_back(m_buff.retrieveAllToStr());
        } else {
            fputs(m_buff.peek(), m_fp);
        }
        m_buff.retrieveAll();
    }
}

void Log::flush(void) {
    {
        lock_guard<mutex> locker(m_mtx);
        fflush(m_fp);
    }
    if(m_isAsync) { 
        m_deque->flush(); 
    }
}

int Log::getLevel() {
    lock_guard<mutex> locker(m_mtx);
    return m_level;
}

void Log::setLevel(int level) {
    lock_guard<mutex> locker(m_mtx);
    m_level = level;
}

void Log::m_appendLogLevelTitle(int level) {
    switch(level) {
    case 0:
        m_buff.append("[debug]: ", 9);
        break;
    case 1:
        m_buff.append("[info] : ", 9);
        break;
    case 2:
        m_buff.append("[warn] : ", 9);
        break;
    case 3:
        m_buff.append("[error]: ", 9);
        break;
    default:
        m_buff.append("[info] : ", 9);
        break;
    }
}

void Log::m_asyncWrite() {
    string str = "";
    while(m_deque->pop(str)) {
        lock_guard<mutex> locker(m_mtx);
        fputs(str.c_str(), m_fp);
    }
}
