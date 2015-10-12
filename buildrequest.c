

void build_request (const char *url) {
    char tmp[10];
    int i;
   
    bzero(host, MAXHOSTNAMELEN);
    bzero(request, REQUEST_SIZE);
   
    /* 协议适配
     * */
    if (force_reload && proxyhost != NULL && http10 < 1) {
        http10=1;
    }
    if (method == METHOD_HEAD && http10 < 1) {
        http10=1;
    }
    if (method == METHOD_OPTIONS && http10 < 2) {
        http10=2;
    }
    if (method == METHOD_TRACE && http10 < 2) {
        http10=2;
    }
   
    switch (method) {
        default:
        case METHOD_GET:
            strcpy(request, "GET");
            break;
        case METHOD_HEAD: 
            strcpy(request, "HEAD");
            break;
        case METHOD_OPTIONS:
            strcpy(request, "OPTIONS");
            break;
        case METHOD_TRACE:
            strcpy(request, "TRACE");
            break;
    }
             
    strcat(request, " ");
   
    if (NULL == strstr(url, "://")) {
        fprintf(stderr, "\n%s: is not a valid URL.\n",url);
        exit(2);
    }
    if (strlen(url) > 1500) {
        fprintf(stderr,"URL is too long.\n");
        exit(2);
    }
    if (proxyhost == NULL) {
        if (0 != strncasecmp("http://", url, 7)) {
            fprintf(stderr,"\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
            exit(2);
        }
    }
   
    /* protocol/host delimiter */
    i = strstr(url, "://") - url + 3;
    /* printf("%d\n",i); */
   
    if (strchr(url + i, '/') == NULL) {
        fprintf(stderr,"\nInvalid URL syntax - hostname don't ends with '/'.\n");
        exit(2);
    }
 
 
    if (proxyhost == NULL) {
        /* get port from hostname */
        if (index(url + i, ':') != NULL && index(url + i, ':') < index(url + i, '/')) {
            strncpy(host, url + i, strchr(url + i, ':') - url - i);
            bzero(tmp, 10);
            strncpy(tmp, index(url + i, ':') + 1, strchr(url + i, '/') - index(url + i, ':') - 1);
            /* printf("tmp=%s\n",tmp); */
            proxyport = atoi(tmp);
            if (proxyport == 0) {
                proxyport=80;
            }
        } else {
            strncpy(host, url + i, strcspn(url + i, "/"));
        }
        // printf("Host=%s\n",host);
        strcat(request + strlen(request), url + i + strcspn(url + i, "/"));
    } else {
        // printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
        strcat(request, url);
    }
 
 
    if (http10 == 1) {
        strcat(request, " HTTP/1.0");
    } else if (http10 == 2) {
        strcat(request, " HTTP/1.1");
    }
    strcat(request,"\r\n");
 
 
    if (http10 > 0) {
        strcat(request, "User-Agent: WebBench "PROGRAM_VERSION"\r\n");
    }
    if (proxyhost == NULL && http10 > 0) {
        strcat(request, "Host: ");
        strcat(request, host);
        strcat(request, "\r\n");
    }
    if (force_reload && proxyhost != NULL) {
        strcat(request, "Pragma: no-cache\r\n");
    }
    if (http10 > 1) {
        strcat(request, "Connection: close\r\n");
    }
    /* add empty line at end */
    if (http10 > 0) {
        strcat(request, "\r\n"); 
    }
    // printf("Req=%s\n",request);
}