#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void makeResponse(Buffer& buff);
    void unmapFile();
    char* file();
    size_t fileLen() const;
    void errorContent(Buffer& buff, std::string message);
    int code() const { return m_code; }

private:
    void m_addStateLine(Buffer &buff);
    void m_addHeader(Buffer &buff);
    void m_addContent(Buffer &buff);

    void m_errorHtml();
    std::string m_getFileType();

    int m_code;
    bool m_isKeepAlive;

    std::string m_path;
    std::string m_srcDir;
    
    char* m_mmFile; 
    struct stat m_mmFileStat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //HTTP_RESPONSE_H