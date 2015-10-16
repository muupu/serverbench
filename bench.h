#ifndef __BENCH__H__
#define __BENCH__H__


/* 子进程执行请求任务的函数 */
void benchcore(const char* host,const int port, const char *request);
/* 执行压测的主要入口函数 */
int bench(void);

#endif // __BENCH__H__