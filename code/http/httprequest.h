#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
    public:
        enum PARSE_STATE {
            REQUEST_LINE,
            HEADERS,
            BODY,
            FINISH,
        };

        enum HTTP_CODE {
            NO_REQUEST = 0,
            GET_REQUEST,
            BAD_REQUEST,
            NO_RESOURSE,
            FORBIDDENT_REQUEST,
            FILE_REQUEST,
            INTERNAL_ERROR,
            CLOSED_CONNECTION,
        };

        HttpRequest() { init(); }
        ~HttpRequest() = default;

        void init();
        bool parse(Buffer& buff);

        std::string path() const;
        std::string& path();
        std::string method() const;
        std::string version() const;
        std::string getPost(const std::string& key) const;
        std::string getPost(const char* key) const;

        bool isKeepAlive() const;

        /* 
        todo 
        void HttpConn::ParseFormData() {}
        void HttpConn::ParseJson() {}
        */

    private:
        bool m_parseRequestLine(const std::string& line);
        void m_parseHeader(const std::string& line);
        void m_parseBody(const std::string& line);

        void m_parsePath();
        void m_parsePost();
        void m_parseFromUrlencoded();

        static bool userVerify(const std::string& name, const std::string& pwd, bool isLogin);

        PARSE_STATE m_state;
        std::string m_method, m_path, m_version, m_body;
        std::unordered_map<std::string, std::string> m_header;
        std::unordered_map<std::string, std::string> m_post;

        static const std::unordered_set<std::string> DEFAULT_HTML;
        static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
        static int converHex(char ch);
};


#endif  //HTTP_REQUEST_H