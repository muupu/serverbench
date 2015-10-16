#ifndef __SERVERBENCH__H__
#define __SERVERBENCH__H__

#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
/* 
 * 超时标记，当被设置为 1 时，所有子进程退出
 * volatile：
 *    - 让系统总是从内存读取数据，
 *    - 告诉编译器不要做任何优化，
 *    - 变量会在程序外被改变
 */
extern volatile int timerexpired;

/* 成功请求数 */
extern int speed ;
/* 失败请求数 */
extern int failed;
/* 读取字节总数 */
extern int bytes;
/* 支持的 HTTP 协议版本*/
extern int http10 ; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */

/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define PROGRAM_VERSION "1.5"

/* 默认为GET方法 */
extern int method;

extern int clients; /* 并发数。默认启动一个客户端（子进程） */
extern int force; /* 是否等待服务器响应数据返回，0 －等待，1 － 不等待 */
extern int force_reload; /* 是否发送 Pragma: no-cache */
extern int proxyport; /* 代理端口 */
extern char *proxyhost; /* 代理服务器名称 */

/*
 * 执行时间，当子进程执行时间到过这个秒数之后，
 * 发送 SIGALRM 信号，将 timerexpired 设置为 1，
 * 让所有子进程退出 
 */
extern int benchtime;

/* 管道，子进程完成任务后，向写端写入数据，主进程从读端读取数据 */
extern int mypipe[2];

extern char host[MAXHOSTNAMELEN]; /* 主机名（64字节） */

#define REQUEST_SIZE 2048
extern char request[REQUEST_SIZE]; /* 请求字符串（HTTP头） */

/* 命令行的选项配置表，细节部分查看man文档：man getopt_long */
extern const struct option long_options[];


#endif //__SERVERBENCH__H__