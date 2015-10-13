
#include "serverbench.h"
#include "socket.h"
#include "buildrequest.h"

volatile int timerexpired=0;
int speed  = 0;
int failed = 0;
int bytes  = 0;
int http10 = 1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
int method = METHOD_GET;
int clients      = 1; /* 并发数。默认启动一个客户端（子进程） */
int force        = 0; /* 是否等待服务器响应数据返回，0 －等待，1 － 不等待 */
int force_reload = 0; /* 是否发送 Pragma: no-cache */
int proxyport    = 80; /* 代理端口 */
char *proxyhost  = NULL; /* 代理服务器名称 */
int benchtime = 30;
int mypipe[2];
char host[MAXHOSTNAMELEN]; /* 主机名（64字节） */
char request[REQUEST_SIZE]; /* 请求字符串（HTTP头） */
const struct option long_options[] = {
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

/* 将 timerexpired 设置为1，让所有子进程退出 */
static void alarm_handler(int signal)
{
   timerexpired=1;
}	

static void usage(void) {
    fprintf(stderr,
        "webbench [option]... URL\n"
        "  -f|--force               Don't wait for reply from server.\n"
        "  -r|--reload              Send reload request - Pragma: no-cache.\n"
        "  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
        "  -p|--proxy <server:port> Use proxy server for request.\n"
        "  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
        "  -9|--http09              Use HTTP/0.9 style requests.\n"
        "  -1|--http10              Use HTTP/1.0 protocol.\n"
        "  -2|--http11              Use HTTP/1.1 protocol.\n"
        "  --get                    Use GET request method.\n"
        "  --head                   Use HEAD request method.\n"
        "  --options                Use OPTIONS request method.\n"
        "  --trace                  Use TRACE request method.\n"
        "  -?|-h|--help             This information.\n"
        "  -V|--version             Display program version.\n"
    );
};

int main(int argc, char *argv[]) {
    int  opt           = 0;
    int  options_index = 0;
    char *tmp          = NULL;
 
 
    if (argc == 1) {
        usage();
        return 2;
    } 
 
 
    /* 参数解释，参见 man getopt_long */
    while ((opt = getopt_long(argc, argv, "912Vfrt:p:c:?h", long_options, &options_index)) != EOF) {
        switch(opt) {
            case  0 :
                break;
            case 'f':
                force = 1;
                break;
            case 'r': 
                force_reload = 1; 
                break; 
            case '9': 
                http10 = 0;
                break;
            case '1':
                http10 = 1;
                break;
            case '2':
                http10 = 2;
                break;
            case 'V':
                printf(PROGRAM_VERSION"\n");
                exit(0);
            case 't':
                benchtime = atoi(optarg);
                break;         
            case 'p': 
                /* proxy server parsing server:port */
                tmp       = strrchr(optarg, ':');
                proxyhost = optarg;
                if (tmp == NULL) {
                    break;
                }
                if (tmp == optarg) {
                    fprintf(stderr,"Error in option --proxy %s: Missing hostname.\n",optarg);
                    return 2;
                }
                if (tmp == optarg + strlen(optarg) - 1) {
                    fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
                    return 2;
                }
                *tmp='\0';
                proxyport = atoi(tmp + 1);
                break;
            case ':':
            case 'h':
            case '?':
                usage();
                return 2;
                break;
            case 'c':
                clients = atoi(optarg);
                break;
        }
    }
  
    if (optind == argc) {
        fprintf(stderr, "webbench: Missing URL!\n");
        usage();
        return 2;
    }
 
    if (clients == 0) {
        clients = 1;
    }
 
    if (benchtime == 0) {
        benchtime = 60;
    }
 
    build_request(argv[optind]); /* 最后一个非选项的参数，被视为URL */
 
    /* print bench info */
    printf("\nBenchmarking: ");
    switch(method) {
        case METHOD_GET:
        default:
            printf("GET");
            break;
        case METHOD_OPTIONS:
            printf("OPTIONS");
            break;
        case METHOD_HEAD:
            printf("HEAD");
            break;
        case METHOD_TRACE:
            printf("TRACE");
            break;
    }
    printf(" %s",argv[optind]);
    switch(http10) {
        case 0: 
            printf(" (using HTTP/0.9)");
            break;
        case 2: 
            printf(" (using HTTP/1.1)");
            break;
    }
    printf("\n");
    if (clients == 1) {
        printf("1 client");
    } else {
        printf("%d clients", clients);
    }
    printf(", running %d sec", benchtime);
    if (force) {
        printf(", early socket close");
    }
    if (proxyhost != NULL) {
        printf(", via proxy server %s:%d", proxyhost, proxyport);
    }
    if (force_reload) {
        printf(", forcing reload");
    }
    printf(".\n");
 
    return bench();
}



/* vraci system rc error kod */
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