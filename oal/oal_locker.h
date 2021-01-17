#ifndef __OAL_LOCKER_H
#define __OAL_LOCKER_H
/*
	文件说明：
	专门用于线程之间同步的三种机制：
	1.POSIX信号量、互斥锁、条件变量封装 
	2.互斥锁：用户同步线程间对共享数据的访问	
*/
#include "../prepare.h"

	class sem{
	public:
		/*创建并初始化信号量*/
		sem(){
			if(sem_init(&m_sem, 0, 0) != 0){
				throw std::exception();
			}
		}
		sem(oal_uint32 val){
			if(sem_init(&m_sem, 0, val) != 0){
				throw std::exception();
			}
		}
		/*销毁信号量*/
		~sem(){
			sem_destroy(&m_sem);
		}
		
		/*等待信号量*/
		bool wait(){
			/*信号量的值减1*/
			return sem_wait(&m_sem) == 0;
		}
		/*增加信号量*/
		bool post(){
			/*信号量的值加1*/
			return sem_post(&m_sem) == 0;
		}
	private:
		sem_t m_sem;
	};

	class mutex{
	public:
		/*创建并初始化互斥锁*/
		mutex(){
			if(pthread_mutex_init(&m_mutex, NULL) != 0){
				throw std::exception();
			}
		}
		/*销毁互斥锁*/	
		~mutex(){
			pthread_mutex_destroy(&m_mutex);
		}
		/*获取互斥锁*/
		bool lock(){
			return pthread_mutex_lock(&m_mutex) == 0;
		}
		/*释放互斥锁*/
		bool unlock(){
			return pthread_mutex_unlock(&m_mutex) == 0;
		}
	private:
		pthread_mutex_t m_mutex;
	};

	class cond{
	public:
		/*创建并初始化条件变量，并创建初始化保护条件变量的互斥锁*/
		cond(){
			if(pthread_mutex_init(&m_mutex, NULL) != 0){
				throw std::exception();
			}
			if(pthread_cond_init(&m_cond, NULL) != 0){
				pthread_mutex_destroy(&m_mutex);
				throw std::exception();
			}		
		}
		/*销毁条件变量和互斥锁*/
		~cond(){
			pthread_mutex_destroy(&m_mutex);
			pthread_cond_destroy(&m_cond);
		}
		/*等待条件变量*/
		bool wait(){
			oal_uint8 ret = 0;
			LOG(LEV_DEBUG, "1\n");
			pthread_mutex_lock(&m_mutex);
			LOG(LEV_DEBUG, "2\n");
			ret = pthread_cond_wait(&m_cond, &m_mutex);
			LOG(LEV_DEBUG, "3\n");
			pthread_mutex_unlock(&m_mutex);
			LOG(LEV_DEBUG, "4\n");
			return ret == 0;
		}
		/*唤醒等待条件变量的线程*/
		bool signal(){
			return pthread_cond_signal(&m_cond) == 0;
		}
	private:
		pthread_cond_t m_cond;
		pthread_mutex_t m_mutex;
	};

	typedef sem       oal_semaphore;
	typedef mutex     oal_mutex_lock;
	typedef cond      oal_condition;



#endif