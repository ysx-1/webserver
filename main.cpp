#include "prepare.h"

int main(){

#ifdef _LOG_TEST
	LOG(LEV_INFO, "log print test\n");
	oal_int64 i = 666666;
	LOG(LEV_INFO, "hello server！(%lld)\n", i);
#endif

#ifdef _LOCKER_TEST
	LOG(LEV_INFO, "locker test\n");
	
	LOG(LEV_INFO, "create semaphore\n");
	oal_semaphore semu(0);
	LOG(LEV_INFO, "wait semaphore\n");
	semu.wait();
	LOG(LEV_INFO, "post semaphore\n");
	semu.post();
	
	LOG(LEV_INFO, "create mutex\n");
    oal_mutex_lock mutex_test;
	LOG(LEV_INFO, "lock mutex\n");
	mutex_test.lock();
	LOG(LEV_INFO, "unlock mutex\n");
	mutex_test.unlock();
	
	LOG(LEV_INFO, "create condition\n");
	oal_condition con_test;
	LOG(LEV_INFO, "signal condition\n");
	con_test.signal();
	LOG(LEV_INFO, "wait condition\n");
	con_test.wait();
	LOG(LEV_INFO, "signal condition\n");
	con_test.signal();
#endif

#ifdef _THREAD_TEST
	pthread_t test_thread_id;
	oal_condition cond_test_thread;
	
	pthread_create(&test_thread_id, NULL, test_thread, (oal_void*)&cond_test_thread);
	
	oal_sleep_s(1);
	LOG(LEV_INFO, "signal condition\n");
	cond_test_thread.signal();
	//oal_sleep_s(1);
	oal_void* pthread_ret;
	oal_int32 pjret = pthread_join(test_thread_id, &pthread_ret);
	
	LOG(LEV_INFO, "[%d],thread return[%d]\n", pjret, (oal_int32*)pthread_ret);
#endif

#ifdef _THREADPOOL_TEST
    threadpool_test thpt;
	threadpool<threadpool_test> m_threadpool;
	oal_bool ret = m_threadpool.append(&thpt);
	LOG(LEV_INFO, "append ret(%d)\n", ret);
    ret = m_threadpool.append(&thpt);
    LOG(LEV_INFO, "append ret(%d)\n", ret);
    ret = m_threadpool.append(&thpt);
    LOG(LEV_INFO, "append ret(%d)\n", ret);

    oal_sleep_s(3);
#endif

#ifdef _AID_LEARNING_TEST
	LOG(LEV_INFO,"Aid learning test888!\n");
#endif

#ifdef _ERRNO_TEST
	LOG_ERRNO("errno test 1");
	oal_int32 oldErrno = errno;
	errno = 2;
	LOG_ERRNO("errno test 2");
	LOG_ERRNO_CLEAN("errno test 3")
	errno = oldErrno;
#endif

#ifdef _REFLECTION_TEST
    reflection_server ref_server;
    ref_server.init(6666, 8);
    ref_server.thread_pool();
    ref_server.eventlisten();
    ref_server.eventloop();
#endif

#ifdef _WEB_SERVER_TEST
	//数据库登录名,密码,库名
	string user = "root";
	string passwd = "4110";
	string databasename = "ysxdb";

	LOG(LEV_INFO,"web_server_test!\n");
	web_server web_server_test;
	web_server_test.init(9999, 5);
	web_server_test.thread_pool();
	web_server_test.eventlisten();
	web_server_test.eventloop();
#endif

	return 0;
}
