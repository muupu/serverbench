#include "bench.h"
#include "serverbench.h"

int bench(void) {
    int   i, j, k;    
    pid_t pid = 0;
    FILE  *f;
   
    /* 测试远程主机是否能够连通 */
    i = Socket(proxyhost == NULL ? host : proxyhost, proxyport);
    if (i < 0) { 
        fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
        return 1;
    }
    close(i);
 
 
    /* 创建管道 */
    if (pipe(mypipe)) {
        perror("pipe failed.");
        return 3;
    }
   
    /* not needed, since we have alarm() in childrens */
    /* wait 4 next system clock tick */
    /*
    cas=time(NULL);
    while(time(NULL)==cas)
          sched_yield();
    */
   
    /* 生成子进程 */
    for (i = 0; i < clients; i++) {
        pid = fork();
        if (pid <= (pid_t)0) {
            /* child process or error*/
            sleep(1); /* make childs faster 快速生成子进程*/
            /*当fork后有2个进程执行。当fork出错或者fork后执行到子进程，就sleep(1)，
            让出CPU，让父进程占用CPU继续执行for循环，fork生成子进程。*/

            /* 这个 break 很重要，它主要让子进程只能从父进程生成，
            否则子进程会再生成子进程，子子孙孙很庞大的 */
            break;
        }
    }
   
    if (pid < (pid_t)0) {
        fprintf(stderr,"problems forking worker no. %d\n",i);
        perror("fork failed.");
        return 3;
    }
   
    if (pid == (pid_t)0) {
        /* I am a child */
        /* 子进程执行请求：尽可能多的发送请求，直到超时返回为止 */
        if(proxyhost == NULL) {
            benchcore(host, proxyport, request);
        } else {
            benchcore(proxyhost, proxyport, request);
        }
        /* write results to pipe */
        f = fdopen(mypipe[1], "w");
        if (f == NULL) {
            perror("open pipe for writing failed.");
            return 3;
        }
        /* fprintf(stderr,"Child - %d %d\n",speed,failed); */
        /* 将子进程执行任务的结果写入到管道中，以便父进程读取 */
        fprintf(f, "%d %d %d\n", speed, failed, bytes);
        fclose(f);
        /* 子进程完成任务，返回退出 */
        return 0;
    } else {
        /* 父进程读取管道，打印结果 */
        printf("parent %d\n", getpid());
        f = fdopen(mypipe[0], "r");
        if (f == NULL) {
            perror("open pipe for reading failed.");
            return 3;
        }
        setvbuf(f, NULL, _IONBF, 0);
        /* 虽然子进程不能污染父进程的这几个变量，但用前重置一下，在这里是个好习惯 */
        speed  = 0;
        failed = 0;
        bytes  = 0;
        /* 从管道中读取每个子进程的任务的执行情况，并计数 */
        while(1) {
            pid = fscanf(f, "%d %d %d", &i, &j, &k);
            if (pid < 2) {
                fprintf(stderr,"Some of our childrens died.\n");
                break;
            }
            speed  += i;
            failed += j;
            bytes  += k;
            /* fprintf(stderr,"*Knock* %d %d read=%d\n",speed,failed,pid); */
            if (--clients == 0) {
                break;
            }
        }
        fclose(f);
 
        /* 打印结果 */
        printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
              (int)((speed + failed) / (benchtime / 60.0f)),
              (int)(bytes / (float)benchtime),
              speed,
              failed);
    }
    return i;
}

void benchcore(const char *host, const int port, const char *req) {
    int    rlen;
    char   buf[1500];
    int    s, i;
    struct sigaction sa;
    
    /* 这个是关键，当程序执行到指定的秒数之后，发送 SIGALRM 信号 */
    sa.sa_handler = alarm_handler;
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL)) {
        exit(3);
    }
    alarm(benchtime);
    
    rlen = strlen(req);
    /* 无限执行请求，直到接收到 SIGALRM 信号将 timerexpired 设置为 1 时 */
    nexttry:
    while (1) {
        if (timerexpired) {
            if (failed > 0) {
                /* fprintf(stderr,"Correcting failed by signal\n"); */
                failed--;
            }
            return;
        }
        /* 连接远程服务器 */
        s = Socket(host, port);
        if (s < 0) {
            failed++;
            continue;
        }
        /* 发送请求 */
        if (rlen != write(s, req, rlen)) {
            failed++;
            close(s);
            continue;
        }
        if (http10 == 0) {
            /* 如果是 http/0.9 则关闭 socket 的写操作 */
            if(shutdown(s, 1)) {
                failed++;
                close(s);
                continue;
            }
        }
        /* 如果等待响应数据返回，则读取响应数据，计算传输的字节数 */
        if (force == 0) {
            /* read all available data from socket */
            while(1) {
                if(timerexpired) {
                    break; 
                }
                i = read(s, buf, 1500);
                /* fprintf(stderr,"%d\n",i); */
                if (i < 0) { 
                    failed++;
                    close(s);
                    goto nexttry;
                } else {
                    if(i==0) {
                        break;
                    } else {
                        bytes += i;
                    }
                }
            }
        }
        /* 关闭连接 */
        if (close(s)) {
            failed++;
            continue;
        }
        /* 成功完成一次请求，并计数，继续下一次相同的请求，直到超时为止 */
        speed++;
    }
}