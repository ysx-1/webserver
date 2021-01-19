#ifndef __OAL_TOP_H
#define __OAL_TOP_H
/*
	文件说明：
	平台标准头文件
*/

#include <cstdio>

//#include <iostream>
/*异常处理*/
#include <exception>
#include <errno.h>

/*线程、信号量相关*/
#include <pthread.h>
#include <semaphore.h>

/*sleep等 头文件*/
#include <unistd.h>

/*查询“文件”状态*/
#include<sys/stat.h>

/*mmap函数相关头文件*/
#include <sys/mman.h>

/*list 头文件*/
#include<list>

/*epoll,fcntl等 相关头文件*/
#include <sys/epoll.h>
#include <fcntl.h>

/*信号相关头文件*/
#include <signal.h>

/*memset函数相关头文件*/
#include <cstring>

/*socket 相关*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*atol相关头文件*/
#include <stdlib.h>

/*最大文件描述符个数*/
#define MAX_FD 65536

#endif