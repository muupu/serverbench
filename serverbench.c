#include "serverbench.h"
#include "socket.h"
#include "buildrequest.h"
#include "bench.h"

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
        "serverbench [option]... URL\n"
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
        fprintf(stderr, "serverbench: Missing URL!\n");
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

