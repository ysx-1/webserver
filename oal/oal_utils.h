//
// Created by 郁帅鑫 on 2021/1/3.
//

#ifndef __OAL_UTILS_H
#define __OAL_UTILS_H
/*
	文件说明：
	1. 事件（epoll）管理
    2. 文件描述符是否可阻塞
    3. 进程的信号处理
*/
#include "../prepare.h"

#define MAX_EVENT_NUMBER 10000

class utils{
public:
    enum PROTO_TYPE{
        PROTO_TCP = 0,
        PROTO_UDP = 1,
        PROTO_MAX = 2
    };
public:
    oal_int32 create_socket(oal_int32 proto_type, oal_bool reuse = true){
        oal_int32 sock = -1;
        oal_int32 protocol = -1;
        oal_int32 reflag = 1;/*socket 可重用标志位*/
        switch (proto_type) {
            case PROTO_TCP:
                LOG(LEV_INFO, "socket protocol(%d)\n", proto_type);
                protocol = SOCK_STREAM;
                break;
            case PROTO_UDP:
                LOG(LEV_INFO, "socket protocol(%d)\n", proto_type);
                protocol = SOCK_DGRAM;
                break;
            default:
                LOG(LEV_ERROR, "recv unknow protocol(%d)\n", proto_type);
                return sock;
                break;
        }
        sock = socket(PF_INET, protocol, 0);
        if(sock == -1){
            LOG(LEV_DEBUG, "socket create failed!\n");
            return sock;
        }
        if (reuse){
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reflag, sizeof(reflag));
        }
        return sock;
    }
    oal_void close_socket(oal_int32 sock){
        LOG(LEV_INFO, "close %d\n", sock);
        close(sock);
    }
    oal_int32 bind_socket(oal_int32 sock, oal_int16 port, oal_uint32 IPstr = INADDR_ANY){
        oal_int32 ret = -1;
        struct sockaddr_in address;

        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(IPstr);
        address.sin_port = htons(port);

        ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
        if (ret != 0){
            LOG(LEV_ERROR, "socket bind failed!\n");
            LOG_ERRNO("socket bind failed, because");
            close(sock);
            return ret;
        }
        return ret;
    }
    oal_int32 listen_socket(oal_int32 sock, oal_int32 backlog = 5){
        oal_int32 ret = -1;
        ret = listen(sock, backlog);
        if (ret != 0){
            LOG(LEV_ERROR, "_socket listen failed!\n");
            close(sock);
            return ret;
        }
        return ret;
    }
    oal_int32 create_pairsocket(oal_int32 proto_type, oal_int32 fd[2]){
        oal_int32 ret = -1;
        oal_int32 protocol = -1;
        switch (proto_type) {
            case PROTO_TCP:
                LOG(LEV_INFO, "socket protocol(%d)\n", proto_type);
                protocol = SOCK_STREAM;
                break;
            case PROTO_UDP:
                LOG(LEV_INFO, "socket protocol(%d)\n", proto_type);
                protocol = SOCK_DGRAM;
                break;
            default:
                LOG(LEV_ERROR, "recv unknow protocol(%d)\n", proto_type);
                return ret;
                break;
        }
        ret = socketpair(PF_UNIX, protocol, 0, fd);
        if (ret != 0){
            LOG(LEV_ERROR, "socketpair create failed!\n");
            return ret;
        }
        return ret;
    }

    oal_void close_pairsocket(int fd[2]){
        LOG(LEV_DEBUG, "close %d, %d\n", fd[0], fd[1]);
        close(fd[0]);
        close(fd[1]);
    }

    /*SUCC:0 ,FAILED:-1*/
    oal_int32 create_epoll(oal_int32 sz){
        m_epoll_fd = epoll_create(sz);
        setnonblocking(m_epoll_fd);
        return m_epoll_fd;
    }
    /*SUCC:0 ,FAILED:-1*/
    oal_int32 addfd(oal_int32 epfd, oal_int32 fd, oal_bool oneshot, oal_int32 trigMode){
        epoll_event event;
        event.data.fd = fd;
        if (trigMode == 1){
            /*epoll_ctl add 默认为LT模式，如需ET需添加EPOLLET类型的事件*/
            event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        } else {
            event.events = EPOLLIN | EPOLLRDHUP;
        }
        if (oneshot){
            event.events |= EPOLLONESHOT;
        }
        /*??????*/
        setnonblocking(fd);
        return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    }
    /*SUCC:0 ,FAILED:-1*/
    oal_int32 addfd(oal_int32 fd, oal_bool oneshot, oal_int32 trigMode){
        epoll_event event;
        event.data.fd = fd;
        if (trigMode == 1){
            /*epoll_ctl add 默认为LT模式，如需ET需添加EPOLLET类型的事件*/
            event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        } else {
            event.events = EPOLLIN | EPOLLRDHUP;
        }
        if (oneshot){
            event.events |= EPOLLONESHOT;
        }
        /*??????*/
        setnonblocking(fd);
        return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    }
    oal_int32 setnonblocking(oal_int32 fd){
        oal_int32 old_option = fcntl(fd, F_GETFL);
        oal_int32 new_option = old_option | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_option;
    }
    oal_int32 removefd(oal_int32 epfd, oal_int32 fd){
        oal_int32 ret = -1;
        epoll_event event;
        event.data.fd = fd;
        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return ret;
    }
    oal_int32 removefd(oal_int32 fd){
        oal_int32 ret = -1;
        epoll_event event;
        event.data.fd = fd;
        ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return ret;
    }
    /*将事件重置为oneshot*/
    oal_int32 modfd(oal_int32 epfd, oal_int32 fd, oal_int32 ev, oal_int32 trigMode){
        epoll_event event;
        event.data.fd = fd;
        if (trigMode == 1)
            event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        else
            event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
        return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
    }
    oal_int32 modfd(oal_int32 fd, oal_int32 ev, oal_int32 trigMode){
        epoll_event event;
        event.data.fd = fd;
        if (trigMode == 1)
            event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        else
            event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
        return epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event);
    }
    /*设置alarm信号到来时间间隔*/
    oal_int32 set_timer_slot(oal_int32 _seconds){
        return alarm(_seconds);
    }
    /*设置信号函数*/
    oal_int32 addsig(oal_int32 sig, oal_void(handler)(oal_int32), oal_bool restart = true){
        struct sigaction sa;
        memset(&sa, '\0', sizeof(sa));
        sa.sa_handler = handler;
        if (restart){
            sa.sa_flags |= SA_RESTART;
        }
        /*??????*/
        sigfillset(&sa.sa_mask);
        return sigaction(sig, &sa, NULL);
    }
    oal_static oal_void sighandler(oal_int32 sig) {
        LOG(LEV_DEBUG, "Enter!\n");
        LOG(LEV_INFO, "recv signal(%d)\n", sig);
        //为保证函数的可重入性，保留原来的errno
        oal_int32 save_errno = errno;
        oal_int32 msg = sig;
        send(m_sig_pipefd, (char *)&msg, 1, 0);
        errno = save_errno;
        LOG(LEV_DEBUG, "Exit!\n");
    }
public:
    oal_static oal_int32 m_epoll_fd;/*内核事件表 epoll 描述符*/
    oal_static oal_int32 m_sig_pipefd;/*sig统一信号源*/
    sort_timer_lst timer_lst;/*基于升序链表的定时器*/
};
oal_int32 utils::m_epoll_fd = -1;
oal_int32 utils::m_sig_pipefd = -1;
#endif //__OAL_UTILS_H
