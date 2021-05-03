#include <unistd.h>
#include "server/webserver.h"

int main() {
    /* 守护进程 后台运行 */
    // daemon(1, 0);
    WebServer server(
        8088, 3, 5000, false,                       /* 网络端口 ET模式 timeoutMS 退出 */
        3306, "root", "admin+-*/", "webserver",     /* Mysql配置（根据自己系统上的数据库配置进行修改） */
        12, 8, false, 0, 1024);                     /* 数据库连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.start();
}