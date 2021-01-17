#ifndef __WEB_SERVER_H
#define __WEB_SERVER_H
/*
	文件说明：
	web服务器类
*/

#include "../prepare.h"

class http_parse{
public:
    oal_static oal_const oal_int32 FILENAME_LEN = 200;
    oal_static oal_const oal_int32 READ_BUFFER_SIZE = 2048;
    oal_static oal_const oal_int32 WRITE_BUFFER_SIZE = 1024;
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE = 5,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE = 0,/*主状态机:解析请求行*/
        CHECK_STATE_HEADER,/*主状态机:解析请求头*/
        CHECK_STATE_CONTENT/*主状态机:解析报文主体*/
    };
    enum LINE_STATUS{
        LINE_OK = 0,/*从状态机:当前行完整且有效*/
        LINE_BAD,/*从状态机:当前行格式有问题*/
        LINE_OPEN/*从状态机:当前行不完整，应该继续读取*/
    };
    enum HTTP_CODE
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST = 5,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum REQUEST_STATE{
        READ = 0,
        WRITE = 1,
        REQUEST_STATE_NUM = 2
    };
public:
    http_parse(){};
    ~http_parse(){};
    oal_void process();
    oal_void init(oal_int32 socket, struct sockaddr_in address);/*供主线程使用，初始化新的请求*/
    oal_void set_request_state(REQUEST_STATE state);/*设置当前用户的状态：读/写*/
private:
    oal_void  init();/*初始化读写Buffer相关变量*/

    /*接收、解析客户端HTTP请求报文，并构造HTTP响应报文相关*/
    oal_bool  recv_client_data();/*接收客户端请求报文*/
    HTTP_CODE process_parse_request();/*解析HTTP请求*/
    oal_bool  process_construct_rsp(HTTP_CODE ret);/*构造HTTP响应*/
    oal_void  process_read_etc();/*处理新的HTTP请求报文入口:接收、解析请求并构造HTTP响应报文*/ 
    oal_void  close_conn(oal_bool real_close = true);/*关闭连接，从epoll表中删除fd，总用户数减1*/
    
    HTTP_CODE parse_request_line(oal_int8 *text);
    HTTP_CODE parse_request_headers(oal_int8 *text);
    HTTP_CODE parse_request_content(oal_int8 *text);
    HTTP_CODE do_request();
    oal_int8  *get_one_line();
    LINE_STATUS step_one_line();/*判断请求的某一行的合法性*/

    oal_bool add_response(oal_const oal_int8 *format, ...);
    oal_bool add_content(oal_const oal_int8 *content);
    oal_bool add_status_line(oal_int32 status, oal_const oal_int8 *title);
    oal_bool add_headers(oal_int32 content_length);
    oal_bool add_content_type();
    oal_bool add_content_length(oal_int32 content_length);
    oal_bool add_linger();
    oal_bool add_blank_line();

    /*发送响应报文以及涉及到的html等文件，到客户端 相关*/
    oal_void unmap();
    oal_bool send_request_rsp();/*发送已经构造完成的HTTP响应报文*/
    oal_void process_write_etc();/*发送已经构造好的HTTP响应入口*/
public:
    oal_static oal_int32 m_user_count;/*当前用户总数，类共有成员，大于MAX_FD时，不再接收新的连接请求*/
private:
    /*sockt、epoll工具*/
    utils m_utils;
    
    /*用户属性相关*/
    oal_int32 m_socket;/*记录当前用户的连接socket*/
    struct sockaddr_in m_address;/*记录当前用户地址信息*/

    /*请求解析及响应相关*/
    /*各变量的详细注释在写函数实现过程中添加*/
    oal_int32 m_request_state;/*标识当前用户的状态:读0/写1*/
    oal_int8 m_read_buf[READ_BUFFER_SIZE];/*保存客户端发来的数据buffer*/
    oal_int32 m_read_idx;/*当前read_buf里数据的总字节数*/
    oal_int32 m_checked_idx;/*即将解析下一行的开头*/
    oal_int32 m_one_line_start;/*当前解析行的开头*/
    oal_int8 m_write_buf[WRITE_BUFFER_SIZE];
    oal_int32 m_write_idx;
    CHECK_STATE m_check_state;/*当前在解析HTTP请求报文的哪一部分,初值设置为CHECK_STATE_REQUESTLINE*/
    
    METHOD m_method;/*记录当前连接，当前HTTP请求的方法，初值设置为GET*/ 
    oal_int8 *m_url;/*记录当前连接，当前HTTP请求的URL*/
    oal_int8 *m_version;/*记录当前连接，当前HTTP请求的HTTP版本号*/
    oal_int8 *m_host;/*记录接收http请求的服务器名称和端口号*/
    
    oal_int32 m_content_length;/*解析请求头时使用，表示报文主体的长度，默认为0*/
    oal_bool m_linger;/*响应报文后是否断开TCP连接，默认为false*/
    oal_int8 m_real_file[FILENAME_LEN];
    oal_int8 *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    oal_int32 m_iv_count;
    oal_int32 cgi;
    oal_int8 *m_string; 
    oal_int32 bytes_to_send;
    oal_int32 bytes_have_send;
    oal_int8 *doc_root;
};
/*类的静态变量初始化*/
oal_int32 http_parse::m_user_count = 0;

oal_void http_parse::process(){
    LOG(LEV_DEBUG, "Enter!\n");
    switch(m_request_state){
    case READ:
        LOG(LEV_INFO, "Fd[%d] current m_request_state[%d]\n", m_socket, m_request_state);
        process_read_etc();
        break;
    case WRITE:
        LOG(LEV_INFO, "Fd[%d] current m_request_state[%d]\n", m_socket, m_request_state);
        process_write_etc();
        break;
    default:
        LOG(LEV_ERROR, "Fd[%d] Recv invalid m_request_state[%d]\n", m_socket, m_request_state);
        break;
    };
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void http_parse::init(oal_int32 socket, struct sockaddr_in address){
    LOG(LEV_DEBUG, "Enter!\n");
    m_socket = socket;
    m_address = address;
    /*用户计数+1*/
    m_user_count++;
    init();
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void http_parse::init(){
    LOG(LEV_DEBUG, "Enter!\n");
    m_read_idx = 0;
    m_checked_idx = 0;
    m_one_line_start = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;

    m_method = GET;
    m_url = NULL;
    m_version = NULL;
    
    m_content_length = 0;
    m_linger = false;
    m_host = NULL;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void http_parse::set_request_state(REQUEST_STATE state){
    LOG(LEV_DEBUG, "Enter!\n");
    m_request_state = state;
    LOG(LEV_INFO, "The state of [%d] is [%d]\n", m_socket, m_request_state);
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_bool http_parse::recv_client_data(){
    LOG(LEV_DEBUG, "Enter!\n");
    oal_bool ret = true;
    oal_int32 bytes_read = 0;
    if(m_read_idx >= READ_BUFFER_MAX){
        LOG(LEV_ERROR, "Failed, buffer is full!\n");
        ret = false;
        goto Failed;
    }
    bytes_read = recv(m_socket, m_read_buf + m_read_idx, READ_BUFFER_MAX - m_read_idx, 0);
    if(bytes_read == 0){
        LOG(LEV_INFO, "current connect[%d] is disconnected\n");
        LOG_ERRNO("ERROR CODE");
        ret = false;
    } else if(bytes_read < 0){
        LOG(LEV_ERROR, "connect[%d] recv error!\n", m_socket);
        ret = false;
    } else {
        m_read_idx += bytes_read;
        LOG(LEV_INFO, "connect[%d] recv [%d] bytes!\n", m_socket, bytes_read);
        ret = true;
    }
Failed:
    LOG(LEV_DEBUG, "Exit!\n");
    return ret;
}
http_parse::HTTP_CODE http_parse::process_parse_request(){
    LOG(LEV_DEBUG, "Enter!\n");
    HTTP_CODE ret = NO_REQUEST;
    LINE_STATUS line_status = LINE_OK;
    oal_int8 *line_text = NULL;
    while(
        (m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK)||
        ((line_status = step_one_line()) == LINE_OK)     
    )
    {
        line_text = get_one_line();
        switch(m_check_state){
        case CHECK_STATE_REQUESTLINE:
            LOG(LEV_INFO, "Fd[%d] is checking Request line\n", m_socket);
            ret = parse_request_line(line_text);
            if(ret == BAD_REQUEST){
                goto Done;
            }
            break;
        case CHECK_STATE_HEADER:
            LOG(LEV_INFO, "Fd[%d] is checking Request header\n", m_socket);
            ret = parse_request_headers(line_text);
            if(ret == BAD_REQUEST){
                goto Done;
            }
            break;
        case CHECK_STATE_CONTENT:
            LOG(LEV_INFO, "Fd[%d] is checking Request content\n", m_socket);
            ret = parse_request_content(line_text);
            break;
        default:
            break;
        }
    }
Done:
    LOG(LEV_DEBUG, "Exit!\n");
    return ret;
}
oal_bool http_parse::process_construct_rsp(HTTP_CODE parse_ret){
    LOG(LEV_DEBUG, "Enter!\n");
    oal_bool ret = true;

    LOG(LEV_DEBUG, "Exit!\n");
    return ret;
}
oal_void http_parse::process_read_etc(){
    LOG(LEV_DEBUG, "Enter!\n");
    oal_bool recv_ret = true;
    oal_bool add_rsp_ret = true;
    HTTP_CODE parse_ret = NO_REQUEST;
    recv_ret = recv_client_data();
    if(recv_ret == false){
        LOG(LEV_ERROR, "recv_client_data failed\n");
        /*接收客户端请求数据失败，关闭连接*/
        close_conn();
        return;
    }
    parse_ret = process_parse_request();
    if(parse_ret == NO_REQUEST){
        LOG(LEV_ERROR, "process_parse_request failed\n");
        m_utils.modfd(m_socket, EPOLLIN, 0);
        return;
    }
    add_rsp_ret = process_construct_rsp(parse_ret);
    if(add_rsp_ret != true){
        /*构建响应报文失败，关闭连接*/
        close_conn();
        return;
    }
    m_utils.modfd(m_socket, EPOLLOUT, 0);
    LOG(LEV_DEBUG, "Exit!\n");
}

oal_void http_parse::close_conn(oal_bool real_close){
    LOG(LEV_DEBUG, "Enter!\n");
    m_utils.close_socket(m_socket);
    m_utils.removefd(m_socket);
    m_user_count--;
    LOG(LEV_DEBUG, "Exit!\n");
}
http_parse::HTTP_CODE http_parse::parse_request_line(oal_int8 *text){
    LOG(LEV_DEBUG, "Enter!\n");
    HTTP_CODE ret = NO_REQUEST;
    /*使m_url指向第一次出现" "或"\t"的位置*/
    m_url = strpbrk(text, " \t");
    if(m_url == NULL){
        ret = BAD_REQUEST;
        LOG(LEV_ERROR, "Fd[%d]'S URL is NULL!\n", m_socket);
        goto Done;
    }
    /*在m_method末尾添加'\0',同时向后移动指针，使其指向m_url*/
    *m_url++ = '\0';

    if(strcasecmp(text, "GET") == 0){
        m_method = GET;
    } else if(strcasecmp(text, "POST") == 0){
        m_method = POST;
    } else {
        ret = BAD_REQUEST;
        LOG(LEV_ERROR, "Fd[%d]'S method is invalid[%s]!\n", m_socket, text);
        goto Done;
    }
    /*跳过多余的" "或"\t"*/
    m_url += strspn(m_url, " \t");

    m_version = strpbrk(m_url, " \t");
    if(m_version == NULL){
        ret = BAD_REQUEST;
        LOG(LEV_ERROR, "Fd[%d]'S m_version is NULL!\n", m_socket);
        goto Done;
    }
    /*在m_url末尾添加'\0',同时向后移动指针，使其指向m_version*/
    *m_version++ = '\0';

    /*跳过多余的" "或"\t"*/
    m_version += strspn(m_version, " \t");

    /*仅支持HTTP/1.1, 在step_one_line函数就已经把行末尾置为'\0'*/
    if(strcasecmp(m_version, "HTTP/1.1") != 0){
        ret = BAD_REQUEST;
        LOG(LEV_ERROR, "Fd[%d]'S m_version is invalid[%s]!\n", m_socket, m_version);
        goto Done;
    }

    if(strncasecmp(m_url, "http://", 7) == 0){
        m_url += 7;
        /*使m_url指向第一次出现'/'的位置*/
        m_url = strchr(m_url, '/');
    } else if(strncasecmp(m_url, "https://", 8) == 0){
        m_url += 8;
        /*使m_url指向第一次出现'/'的位置*/
        m_url = strchr(m_url, '/');
    }
    /*这种做法，会使得http://www.lab.glasscom.com 这种URL无法显示，即末尾至少有一个/*/
    if(m_url == NULL || m_url[0] != '/'){
        ret = BAD_REQUEST;
        LOG(LEV_ERROR, "Fd[%d]'S file path is invalid[%s]!\n", m_socket, m_url);
        goto Done;
    }
    if(strlen(m_url) == 1){
        strcat(m_url, "default.html");
    }
    /*HTTP请求行解析完成，更新check状态*/
    m_check_state = CHECK_STATE_HEADER;
Done:
    LOG(LEV_DEBUG, "Exit!\n");
    return ret;
}
http_parse::HTTP_CODE http_parse::parse_request_headers(oal_int8 *text){
    LOG(LEV_DEBUG, "Enter!\n");
    /*该函数会多次进入，因为请求头有多行数据，而我们是以行为单位解析的*/
    HTTP_CODE ret = NO_REQUEST;
    if(text[0] == '\0'){
        if(m_content_length != 0){
            m_checked_idx = CHECK_STATE_CONTENT;
            ret = NO_REQUEST;
            goto Done;
        } else {
            ret = GET_REQUEST;
            goto Done;
        }
    } else if(strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        /*跳过多余的' '或'\t'*/
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0){
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        /*跳过多余的' '或'\t'*/
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        /*跳过多余的' '或'\t'*/
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        LOG(LEV_ERROR, "oop!unknow header: %s", text);
    }
Done:
    LOG(LEV_DEBUG, "Exit!\n");
    return ret;
}
http_parse::HTTP_CODE http_parse::parse_request_content(oal_int8 *text){
    LOG(LEV_DEBUG, "Enter!\n");
    HTTP_CODE ret = NO_REQUEST;

    LOG(LEV_DEBUG, "Exit!\n");
    return ret;
}
oal_int8 * http_parse::get_one_line(){
    LOG(LEV_DEBUG, "Enter!\n");
    oal_int8 *line_start = m_read_buf + m_one_line_start;
    LOG(LEV_INFO, "Fd[%d] is parsing line[%s]\n", m_socket, line_start);
    /*更新为下一行的起始位置*/
    m_one_line_start = m_checked_idx;
    LOG(LEV_DEBUG, "Exit!\n");
    return line_start; 
};
http_parse::LINE_STATUS http_parse::step_one_line(){
    LOG(LEV_DEBUG, "Enter!\n");
    LINE_STATUS parse_line_status = LINE_OK;
    oal_int8 curSymbol;
    for(; m_checked_idx < m_read_idx; m_checked_idx++){
        curSymbol = m_read_buf[m_checked_idx];
        if(curSymbol == '\r'){
            if(m_checked_idx + 1 >= m_read_idx){
                parse_line_status = LINE_OPEN;
                goto Done;
            } else if(m_read_buf[m_checked_idx + 1] != '\n'){
                parse_line_status = LINE_BAD;
                goto Done;
            } else {
                parse_line_status = LINE_OK;
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                goto Done;
            }
        }

        /*用来解析上次的非完整行*/
        if(curSymbol == '\n'){
            if(m_checked_idx > 0 && m_read_buf[m_checked_idx - 1] == '\r'){
                parse_line_status = LINE_OK;
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                goto Done;
            } else {
                parse_line_status = LINE_BAD;
                goto Done;
            }
        }
    }
Done:
    LOG(LEV_DEBUG, "Exit!\n");
    return parse_line_status;
}

oal_void http_parse::process_write_etc(){
    LOG(LEV_DEBUG, "Enter!\n");

    LOG(LEV_DEBUG, "Exit!\n");
}

class web_server{
public:
    web_server();
    ~web_server();
    oal_void init(oal_int16 port, oal_int32 thread_nums);
    oal_void thread_pool();/*初始化线程池*/
    oal_void eventlisten();/*创建socket，绑定端口号，开始监听*/
    oal_void eventloop();/*循环处理请求*/
private:
    oal_bool dealsignal(oal_bool &eventLoop_stop);
    oal_bool dealclientdata();
private:
    /*线程池相关*/
    oal_bool poolIsInit;
    threadpool<http_parse> *m_pool;
    oal_int32 m_thread_nums;
    /*用户描述符*/
    http_parse *users;

    /*监听socket相关*/
    oal_int16 m_port;
    oal_int32 m_listen_fd;
    oal_int32 m_sig_pipefd[2];

    /*epoll 相关*/
    utils m_utils;
    epoll_event events[MAX_EVENT_NUMBER];

    /*eventloop 停止标志位*/
    oal_bool m_eventloop_stop;
};

web_server::web_server() {
    LOG(LEV_DEBUG, "Enter!\n");
    poolIsInit = false;
    users = new http_parse[MAX_FD]; 
    LOG(LEV_DEBUG, "Exit!\n");
}
web_server::~web_server() {
    LOG(LEV_DEBUG, "Enter!\n");
    delete []users;
    if (poolIsInit) delete m_pool;
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void web_server::init(oal_int16 port, oal_int32 thread_nums) {
    LOG(LEV_DEBUG, "Enter!\n");
    m_port = port;
    m_thread_nums = thread_nums;
    LOG(LEV_INFO, "m_port = [%d]\n", m_port);
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void web_server::thread_pool() {
    LOG(LEV_DEBUG, "Enter!\n");
    m_pool = new threadpool<http_parse>(m_thread_nums);
    poolIsInit = true;
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void web_server::eventlisten() {
    LOG(LEV_DEBUG, "Enter!\n");
    oal_int32 ret = -1;
    oal_int32 reflag = 1;/*socket 可重用标志位*/
    LOG(LEV_DEBUG, "Enter!\n");
    /*TCP*/
    m_listen_fd = m_utils.create_socket(m_utils.PROTO_TCP);
    if(m_listen_fd == -1){
        LOG(LEV_ERROR, "m_listen_fd create failed!\n");
        return;
    }

    ret = m_utils.bind_socket(m_listen_fd, m_port);
    if (ret != 0){
        LOG(LEV_ERROR, "m_listen_fd bind failed!\n");
        return;
    }
    ret = m_utils.listen_socket(m_listen_fd);
    if (ret != 0){
        LOG(LEV_ERROR, "m_listen_fd listen failed!\n");
        return;
    }

    m_utils.create_epoll(5);

    m_utils.addfd(m_listen_fd, true, 0);

    /*统一 signal 事件源*/
    ret = m_utils.create_pairsocket(m_utils.PROTO_TCP, m_sig_pipefd);//socketpair(PF_UNIX, SOCK_STREAM, 0, m_sig_pipefd);
    m_utils.setnonblocking(m_sig_pipefd[1]);
    m_utils.addfd(m_sig_pipefd[0], false, 0);
    m_utils.m_sig_pipefd = m_sig_pipefd[1];
    /*屏蔽SIGPIPE信号*/
    m_utils.addsig(SIGPIPE, SIG_IGN);
    m_utils.addsig(SIGINT, m_utils.sighandler, true);

    LOG(LEV_DEBUG, "Exit!\n");
}
oal_void web_server::eventloop() {
    m_eventloop_stop = false;
    oal_int32 numbers = 0;
    oal_int32 sockfd = -1;

    LOG(LEV_DEBUG, "Enter!\n");
    while (!m_eventloop_stop){
        LOG(LEV_DEBUG, "EPOLL Wait!\n");
        numbers = epoll_wait(m_utils.m_epoll_fd, events, sizeof(events), -1);
        if (numbers <  0 && errno != EINTR){
            LOG(LEV_ERROR, "epoll_wait failed!\n");
            break;
        }
        for (oal_int32 i = 0; i < numbers; ++i) {
            sockfd = events[i].data.fd;
            if (sockfd == m_listen_fd){
                /*TCP 新连接*/
                LOG(LEV_INFO, "TCP new client arrive!\n");
                oal_bool flag = dealclientdata();
                if (flag == false){
                    LOG(LEV_ERROR, "TCP accept client failed!\n");
                }
            } else if(sockfd == m_sig_pipefd[0] && events[i].events & EPOLLIN){
                /*处理信号*/
                oal_bool flag = dealsignal(m_eventloop_stop);
                if (flag == false){
                    LOG(LEV_ERROR, "dealsignal failed!\n");
                }
            } else if (events[i].events & (EPOLLRDHUP | EPOLLERR)){
                /*TCP client异常断开*/
                LOG(LEV_ERROR, "TCP client close the connect\n");
                m_utils.close_socket(sockfd);//close(sockfd);
                m_utils.removefd(sockfd);
            } else if (events[i].events & EPOLLIN){
                /*TCP 可读*/
                m_pool->append(users + sockfd);
            } else {
                LOG(LEV_WARN, "something else happened \n");
            }
        }
        LOG(LEV_DEBUG, "the end of once epoll_wait\n");
    }
    m_utils.close_socket(m_listen_fd);
    m_utils.close_socket(m_utils.m_epoll_fd);
    m_utils.close_pairsocket(m_sig_pipefd);
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_bool web_server::dealsignal(oal_bool &eventLoop_stop) {
    oal_int32 ret = -1;
    oal_int8 sigbuffer[1024];
    ret = recv(m_sig_pipefd[0], sigbuffer, sizeof(sigbuffer), 0);
    if (ret == -1){
        return false;
    } else if (ret == 0){
        return false;
    } else {
        for (oal_int32 i = 0; i < ret; i++){
            switch (sigbuffer[i]) {
                case SIGINT:
                    LOG(LEV_WARN, "recv Ctrl+C signal(%d)\n", sigbuffer[i]);
                    eventLoop_stop = true;
                    break;
                default:
                    LOG(LEV_WARN, "recv other signal(%d)\n", sigbuffer[i]);
                    break;
            }
        }
        return true;
    }
}
oal_bool web_server::dealclientdata() {
    oal_int32 confd = -1;
    struct sockaddr_in client_addr;
    socklen_t addr_len = 0;
    confd = accept(m_listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (confd < 0){
        return false;
    } else {
        m_utils.addfd(confd, true, 0);
        oal_int8 remote[INET_ADDRSTRLEN];
        LOG(LEV_INFO, "connected client [%s:%d]\n", inet_ntop(AF_INET, &client_addr.sin_addr,remote, INET_ADDRSTRLEN),
                ntohs(client_addr.sin_port));
        users[confd].init(confd, client_addr);
        /*重置m_tcp_socket 为oneshot*/
        m_utils.modfd(m_listen_fd, EPOLLIN, 0);
    }
    return true;
}

#endif