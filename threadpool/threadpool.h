#ifndef __THREADPOOL_H
#define __THREADPOOL_H
/*
	文件说明：
	线程池相关 
*/
#include "../prepare.h"

#ifdef _THREAD_TEST
	oal_void* test_thread(oal_void *arg){
		oal_static oal_int32 ret = 9090;
		oal_condition *cond = (oal_condition *) arg;
		LOG(LEV_INFO, "+wait condition\n");
		cond -> wait();
		LOG(LEV_INFO, "-wait condition\n");
		//return (oal_void*)ret;
		//pthread_exit((oal_void*)ret);
		pthread_exit((oal_void*)6060);
	}
#endif

#ifdef _THREADPOOL_TEST
class threadpool_test{
public:
    oal_void process(){
        LOG(LEV_INFO,"Enter!\n")
        LOG(LEV_INFO, "count = %d\n", this->count++);
        LOG(LEV_INFO,"Exit!\n")
    }
private:
    oal_static oal_uint32 count;
};
oal_uint32 threadpool_test::count = 0;
#endif

template <typename T>
class threadpool{
public:
    threadpool(oal_uint32 thread_number = 8, oal_uint32 max_requests = 1000);
    ~threadpool(){
        delete [] m_threads;
        m_stop = true;
        /*退出被信号量阻塞的线程*/
        m_queuestat.post();
        LOG(LEV_DEBUG, "End!\n");
    }
    oal_bool append(T *request);
private:
    oal_static oal_void* worker(oal_void *arg);
    oal_void run(oal_void);
private:
    oal_uint32 m_thread_number;/*线程池中的线程总数*/
    oal_uint32 m_max_requests;/*请求队列中允许的最大请求数目*/
    pthread_t *m_threads;/*描述线程池的数组，其大小为m_thread_number*/
    std::list< T* > m_workqueue;/*请求队列*/
    oal_mutex_lock m_queuelocker;/*保护请求队列的互斥锁*/
    oal_semaphore m_queuestat;/*是否有任务请求处理*/
    oal_bool m_stop;/*是否结束线程*/

    /*utils 工具集*/
    //utils m_utils;
};
template <typename T>
threadpool<T>::threadpool(oal_uint32 thread_number, oal_uint32 max_requests) :
    m_thread_number(thread_number),
    m_max_requests(max_requests),
    m_threads(NULL),
    m_stop(false)
{
    if (thread_number == 0 || max_requests == 0){
        throw std::exception();
    }
    m_threads = new pthread_t [m_thread_number];
    if (m_threads == NULL){
        throw std::exception();
    }
    for (oal_uint32 i = 0; i < m_thread_number; ++i) {
        LOG(LEV_INFO, "create %dth thread!\n", i);
        oal_int32 ret = pthread_create(&m_threads[i], NULL, worker, this);
        if (ret != 0){
            delete [] m_threads;
            throw std::exception();
        }
        ret = pthread_detach(m_threads[i]);
        if (ret != 0){
            delete [] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
oal_void* threadpool<T>::worker(oal_void *arg){
    threadpool * pool = (threadpool*) arg;
    pool->run();
    return pool;
}
template <typename T>
oal_void threadpool<T>::run(oal_void){
    oal_int32 index = 0;
    while(!m_stop){
        m_queuestat.wait();
        LOG(LEV_DEBUG, "[%lu] is wakened %dth!\n", pthread_self(),index++);
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            /*解锁，等待下一次被唤醒*/
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if( ! request) {
            LOG(LEV_ERROR, "A request in the front of queue is NULL\n");
            continue;
        }
        LOG(LEV_DEBUG, "Begin process request!\n");
        request->process();
    }
}
template <typename T>
oal_bool threadpool<T>::append(T *request){
    LOG(LEV_DEBUG, "Enter!\n");
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    /*唤醒一个工作线程去处理请求*/
    m_queuestat.post();
    return true;
}
#endif