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
volatile int timerexpired=0;

/* 成功请求数 */
int speed  = 0;
/* 失败请求数 */
int failed = 0;
/* 读取字节总数 */
int bytes  = 0;
/* 支持的 HTTP 协议版本*/
int http10 = 1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */

/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define PROGRAM_VERSION "1.5"

/* 默认为GET方法 */
int method = METHOD_GET;

int clients      = 1; /* 并发数。默认启动一个客户端（子进程） */
int force        = 0; /* 是否等待服务器响应数据返回，0 －等待，1 － 不等待 */
int force_reload = 0; /* 是否发送 Pragma: no-cache */
int proxyport    = 80; /* 代理端口 */
char *proxyhost  = NULL; /* 代理服务器名称 */

/*
 * 执行时间，当子进程执行时间到过这个秒数之后，
 * 发送 SIGALRM 信号，将 timerexpired 设置为 1，
 * 让所有子进程退出 
 */
int benchtime = 30;

/* 管道，子进程完成任务后，向写端写入数据，主进程从读端读取数据 */
int mypipe[2];

char host[MAXHOSTNAMELEN]; /* 主机名（64字节） */

#define REQUEST_SIZE 2048
char request[REQUEST_SIZE]; /* 请求字符串（HTTP头） */

/* 命令行的选项配置表，细节部分查看man文档：man getopt_long */
static const struct option long_options[] = {
    {"force",   no_argument,       &force,        1},
    {"reload",  no_argument,       &force_reload, 1},
    {"time",    required_argument, NULL,          't'},
    {"help",    no_argument,       NULL,          '?'},
    {"http09",  no_argument,       NULL,          '9'},
    {"http10",  no_argument,       NULL,          '1'},
    {"http11",  no_argument,       NULL,          '2'},
    {"get",     no_argument,       &method,       METHOD_GET},
    {"head",    no_argument,       &method,       METHOD_HEAD},
    {"options", no_argument,       &method,       METHOD_OPTIONS},
    {"trace",   no_argument,       &method,       METHOD_TRACE},
    {"version", no_argument,       NULL,          'V'},
    {"proxy",   required_argument, NULL,          'p'},
    {"clients", required_argument, NULL,          'c'},
    {NULL,      0,                 NULL,          0}
};

/* prototypes */
/* 子进程执行请求任务的函数 */
void benchcore(const char* host,const int port, const char *request);
/* 执行压测的主要入口函数 */
int bench(void);

#endif //__SERVERBENCH__H__