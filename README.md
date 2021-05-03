# Web-Server

在Linux下用C++实现的Web服务器

## 特点

+ 利用IO复用技术Epoll与线程池实现多线程的高并发Reactor模型

+ 利用正则与状态机解析HTTP请求报文，实现Get和Post请求方法

+ 利用标准库容器封装char，实现自动增长的缓冲区

+ 基于小根堆实现的定时器，关闭超时的非活动连接

+ 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态

+ 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能

## 使用

+ 在Linux上先配置好Mysql数据库

Mysql端口默认3306即可，Mysql用户直接使用root，Mysql密码自己设置，再使用下面方法设置一个数据库

```SQL
//创建yourdb数据库
CREATE DATABASE yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('username', 'password');
```

+ 修改main.cpp中Mysql配置

+ build代码（在项目根目录下）

```bash
make
```

+ 运行server（在项目根目录下）

```bash
./bin/server
```

## 单元测试

```bash
cd test
make
./test
```

## 压力测试

本地压力测试，ip为127.0.0.0，端口为main.cpp中自行设置的网络端口

```bash
./webbench-1.5/webbench -c 100 -t 10 http://ip:port/
./webbench-1.5/webbench -c 1000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```

![image](https://github.com/Ayonveig/Web-Server/blob/main/readme.assest/pressure.png)

测试结果因测试环境而不同

## 改进

使用协程来对异步日志的读写进行改进
