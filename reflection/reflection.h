//
// Created by 郁帅鑫 on 2021/1/4.
//

#ifndef __REFLECTION_H
#define __REFLECTION_H
/*
	文件说明：
	回射服务器类
*/

#include "../prepare.h"

/*接收缓冲区最大字节数*/
#define READ_BUFFER_MAX 2048

class reflection{
public:
    enum PROTO_TYPE{
        PROTO_TCP = 0,
        PROTO_UDP = 1,
        PROTO_MAX = 2
    };
public:
    oal_void process(){
        LOG(LEV_DEBUG, "Enter!\n");
        switch(m_proto_type){
            case PROTO_TCP:
                LOG(LEV_INFO, "the protocol of the user is TCP(%d)\n", m_proto_type);
                dealwithtcp();
                break;
            case PROTO_UDP:
                LOG(LEV_INFO, "the protocol of the user is UDP(%d)\n", m_proto_type);
                dealwithudp();
                break;
            default:
                LOG(LEV_INFO, "the protocol of the user is invalid(%d)\n", m_proto_type);
                break;
        };
        LOG(LEV_DEBUG, "Exit!\n");
    }
    oal_void init(oal_int32 socket, oal_int32 proto_type, struct sockaddr_in address){
        m_socket = socket;
        m_proto_type = proto_type;
        m_address = address;
    }
    oal_void init(oal_int32 socket, oal_int32 proto_type){
        m_socket = socket;
        m_proto_type = proto_type;
    }
private:
    oal_void dealwithtcp(){
        LOG(LEV_DEBUG, "Enter!\n");
        /*缺少错误处理*/

        /*接收数据*/
        buffer_len = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        buffer[buffer_len] = '\0';
        LOG(LEV_INFO, "recv msg from client [%d](%s)\n", buffer_len, buffer);
        /*回传数据*/
        buffer_len = send(m_socket, buffer, buffer_len, 0);
        LOG(LEV_INFO, "send to client [%d](%s)\n", buffer_len, buffer);

        /*重置为oneshot*/
        m_utils.modfd(m_socket, EPOLLIN, 0);
        //TMP
        oal_sleep_ms(1000);
        LOG(LEV_DEBUG, "Exit!\n");
    }
    oal_void dealwithudp(){
        LOG(LEV_DEBUG, "Enter!\n");
        /*缺少错误处理*/

        /*接收数据*/
        struct sockaddr_in clientAddr;
        socklen_t addrlen = sizeof(clientAddr);
        buffer_len = recvfrom(m_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&clientAddr, &addrlen);
        buffer[buffer_len] = '\0';
        LOG(LEV_INFO, "recv msg from client [%d](%s)\n", buffer_len, buffer);
        /*回传数据*/
        buffer_len = sendto(m_socket, buffer, buffer_len, 0, (struct sockaddr*)&clientAddr, addrlen);
        LOG(LEV_INFO, "send to client [%d](%s)\n", buffer_len, buffer);

        /*重置为oneshot*/
        m_utils.modfd(m_socket, EPOLLIN, 0);
        //TMP
        oal_sleep_ms(1000);
        LOG(LEV_DEBUG, "Exit!\n");
    }

private:
    utils m_utils;
    oal_int32 m_proto_type;/*记录当前用户的协议类型TCP/UDP*/
    oal_int32 m_socket;/*记录当前用户的socket*/
    struct sockaddr_in m_address;/*记录当前用户地址信息*/
    oal_uint8 buffer[READ_BUFFER_MAX];
    oal_int32 buffer_len;
};

class reflection_server{
public:
    reflection_server();
    ~reflection_server();
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
    threadpool<reflection> *m_pool;
    oal_int32 m_thread_nums;

    /*用户描述符*/
    reflection *users;

    /*监听socket相关*/
    oal_int16 m_port;
    oal_int32 m_tcp_socket;
    oal_int32 m_udp_socket;

    oal_int32 m_sig_pipefd[2];

    /*epoll 相关*/
    utils m_utils;
    epoll_event events[MAX_EVENT_NUMBER];

    /*eventloop 停止标志位*/
    oal_bool m_eventloop_stop;
};

reflection_server::reflection_server() {
    poolIsInit = false;
    users = new reflection[MAX_FD];
}
reflection_server::~reflection_server() {
    delete []users;
    if (poolIsInit) delete m_pool;
}
oal_void reflection_server::init(oal_int16 port, oal_int32 thread_nums) {
    m_port = port;
    m_thread_nums = thread_nums;
}
oal_void reflection_server::thread_pool() {
    m_pool = new threadpool<reflection>(m_thread_nums);
    poolIsInit = true;
}
oal_void reflection_server::eventlisten() {
    oal_int32 ret = -1;
    oal_int32 reflag = 1;/*socket 可重用标志位*/
    LOG(LEV_DEBUG, "Enter!\n");
    /*TCP*/
    m_tcp_socket = m_utils.create_socket(m_utils.PROTO_TCP);
    if(m_tcp_socket == -1){
        LOG(LEV_ERROR, "m_tcp_socket create failed!\n");
        return;
    }

    ret = m_utils.bind_socket(m_tcp_socket, m_port);
    if (ret != 0){
        LOG(LEV_ERROR, "m_tcp_socket bind failed!\n");
        return;
    }
    ret = m_utils.listen_socket(m_tcp_socket);
    if (ret != 0){
        LOG(LEV_ERROR, "m_tcp_socket listen failed!\n");
        return;
    }
    /*UDP*/
    m_udp_socket = m_utils.create_socket(m_utils.PROTO_UDP);
    if(m_udp_socket == -1){
        LOG(LEV_ERROR, "m_udp_socket create failed!\n");
        return;
    }

    ret = m_utils.bind_socket(m_udp_socket, m_port);
    if (ret != 0){
        LOG(LEV_ERROR, "m_udp_socket bind failed!\n");
        //close(m_udp_socket);
        return;
    }


    m_utils.create_epoll(5);

    m_utils.addfd(m_tcp_socket, true, 0);
    m_utils.addfd(m_udp_socket, true, 0);

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
oal_void reflection_server::eventloop() {
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
        for (int i = 0; i < numbers; ++i) {
            sockfd = events[i].data.fd;
            if (sockfd == m_tcp_socket){
                /*TCP 新连接*/
                LOG(LEV_INFO, "TCP new client arrive!\n");
                oal_bool flag = dealclientdata();
                if (flag == false){
                    LOG(LEV_ERROR, "TCP accept client failed!\n");
                }
            } else if(sockfd == m_udp_socket){
                /*UDP处理*/
                reflection udpref;
                udpref.init(m_udp_socket, reflection::PROTO_UDP);
                m_pool->append(&udpref);
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
    m_utils.close_socket(m_tcp_socket);
    m_utils.close_socket(m_udp_socket);
    m_utils.close_socket(m_utils.m_epoll_fd);
    m_utils.close_pairsocket(m_sig_pipefd);
    LOG(LEV_DEBUG, "Exit!\n");
}
oal_bool reflection_server::dealsignal(oal_bool &eventLoop_stop) {
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
oal_bool reflection_server::dealclientdata() {
    oal_int32 confd = -1;
    struct sockaddr_in client_addr;
    socklen_t addr_len = 0;
    confd = accept(m_tcp_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (confd < 0){
        return false;
    } else {
        m_utils.addfd(confd, true, 0);
        oal_int8 remote[INET_ADDRSTRLEN];
        LOG(LEV_INFO, "connected client [%s:%d]\n", inet_ntop(AF_INET, &client_addr.sin_addr,remote, INET_ADDRSTRLEN),
                ntohs(client_addr.sin_port));
        users[confd].init(confd, reflection::PROTO_TCP, client_addr);
        /*重置m_tcp_socket 为oneshot*/
        m_utils.modfd(m_tcp_socket, EPOLLIN, 0);
    }
    return true;
}

#endif //__REFLECTION_H
