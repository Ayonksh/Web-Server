#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 500, "Internal Error" },
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
    { 500, "/500.html" },
};

HttpResponse::HttpResponse() {
    m_code = -1;
    m_isKeepAlive = false;
    m_path = m_srcDir = "";
    m_mmFile = nullptr; 
    m_mmFileStat = { 0 };
};

HttpResponse::~HttpResponse() {
    unmapFile();
}

void HttpResponse::init(const string& srcDir, string& path, bool isKeepAlive, int code){
    assert(srcDir != "" && path != "");
    if(m_mmFile) { unmapFile(); }
    m_code = code;
    m_isKeepAlive = isKeepAlive;
    m_path = path;
    m_srcDir = srcDir;
    m_mmFile = nullptr; 
    m_mmFileStat = { 0 };
}

void HttpResponse::makeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    if(stat((m_srcDir + m_path).data(), &m_mmFileStat) < 0 || S_ISDIR(m_mmFileStat.st_mode)) {
        m_code = 404;
    }
    else if(!(m_mmFileStat.st_mode & S_IROTH)) {
        m_code = 403;
    }
    else if(m_code == -1) { 
        m_code = 200; 
    }
    m_errorHtml();
    m_addStateLine(buff);
    m_addHeader(buff);
    m_addContent(buff);
}

void HttpResponse::unmapFile() {
    if(m_mmFile) {
        munmap(m_mmFile, m_mmFileStat.st_size);
        m_mmFile = nullptr;
    }
}

char* HttpResponse::file() {
    return m_mmFile;
}

size_t HttpResponse::fileLen() const {
    return m_mmFileStat.st_size;
}

void HttpResponse::errorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(m_code) == 1) {
        status = CODE_STATUS.find(m_code)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(m_code) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";

    buff.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}

void HttpResponse::m_addStateLine(Buffer& buff) {
    string status;
    if(CODE_STATUS.count(m_code) == 1) {
        status = CODE_STATUS.find(m_code)->second;
    }
    else {
        m_code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + to_string(m_code) + " " + status + "\r\n");
}

void HttpResponse::m_addHeader(Buffer& buff) {
    buff.append("Connection: ");
    if(m_isKeepAlive) {
        buff.append("Keep-Alive\r\n");
        buff.append("Keep-Alive: timeout=10000\r\n");
    } else{
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + m_getFileType() + "\r\n");
}

void HttpResponse::m_addContent(Buffer& buff) {
    int srcFd = open((m_srcDir + m_path).data(), O_RDONLY);
    if(srcFd < 0) { 
        errorContent(buff, "File NotFound!");
        return; 
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (m_srcDir + m_path).data());
    int* mmRet = (int*)mmap(0, m_mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return; 
    }
    m_mmFile = (char*)mmRet;
    close(srcFd);
    buff.append("Content-length: " + to_string(m_mmFileStat.st_size) + "\r\n\r\n");
}

void HttpResponse::m_errorHtml() {
    if(CODE_PATH.count(m_code) == 1) {
        m_path = CODE_PATH.find(m_code)->second;
        stat((m_srcDir + m_path).data(), &m_mmFileStat);
    }
}

string HttpResponse::m_getFileType() {
    /* 判断文件类型 */
    string::size_type idx = m_path.find_last_of('.');
    if(idx == string::npos) {
        return "text/plain";
    }
    string suffix = m_path.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

