#ifndef __OAM_BLOCKQUEUE_H
#define __OAM_BLOCKQUEUE_H
/*
	文件说明：
	阻塞队列相关 
*/

#include "../prepare.h"

template<typename T>
class blockQueue{
public:
	blockQueue(oal_int32 maxsize = 1000);
	~blockQueue();
	oal_bool Push(oal_const T &element);
	oal_bool Pop(T &front);
	oal_void show();
	oal_void showArray();
private:
	oal_int32 advanceIndex(oal_int32 cur, oal_int32 offset){
		return (cur + offset + (m_max_size + 1)) % (m_max_size + 1);
	}
	oal_bool isEmpty(){
		return m_head == m_tail;
	}
	oal_bool isFull(){
		return advanceIndex(m_tail, 1) == m_head;
	}
	oal_int32 getSize(){
		return ((m_tail - m_head) + (m_max_size + 1)) % (m_max_size + 1);
	}
	T getFront(){
		if(isEmpty()){
			MT_LOG(LEV_WARN, "BlockQueue is NULL!\n");
		}
		return m_array[m_head];
	}
	T getBack(){
		if(isEmpty()){
			MT_LOG(LEV_WARN, "BlockQueue is NULL!\n");
		}
		return m_array[advanceIndex(m_tail, -1)];
	}
private:
	oal_int32 m_head;
	oal_int32 m_tail;
	T *m_array;
	oal_int32 m_size;
    oal_int32 m_max_size;
    oal_mutex_lock m_blockQueMutex;
	oal_condition m_blockQueueCon;
};

template<typename T>
blockQueue<T>::blockQueue(oal_int32 maxsize){
	MT_LOG(LEV_DEBUG, "Enter\n");

	m_size = 0;
	m_head = 0;
	m_tail = 0;

	m_max_size = maxsize;
	m_array = new T[m_max_size + 1];

	MT_LOG(LEV_DEBUG, "Exit\n");
}

template<typename T>
blockQueue<T>::~blockQueue(){
	MT_LOG(LEV_DEBUG, "Enter\n");

	delete m_array;
	m_array = NULL;

	MT_LOG(LEV_DEBUG, "Exit\n");
}

template<typename T>
oal_bool blockQueue<T>::Push(oal_const T &element){
	MT_LOG(LEV_DEBUG, "Enter\n");
	oal_bool ret = true;
	if(isFull()){
		MT_LOG(LEV_WARN, "BlockQueue is Full!\n");
		return false;
	}
	m_array[m_tail] = element;
	m_tail = advanceIndex(m_tail, 1); 
	m_size++;

	MT_LOG(LEV_DEBUG, "Exit\n");
	return ret;
}

template<typename T>
oal_bool blockQueue<T>::Pop(T &front){
	MT_LOG(LEV_DEBUG, "Enter\n");
	oal_bool ret = true;
	if(isEmpty()){
		MT_LOG(LEV_WARN, "BlockQueue is NULL!\n");
		return false;
	}
	front = m_array[m_head];
	m_head = advanceIndex(m_head, 1);
	m_size--;

	MT_LOG(LEV_DEBUG, "Exit\n");
	return ret;
}
template<typename T>
oal_void blockQueue<T>::show(){
	MT_LOG(LEV_DEBUG, "Enter\n");
	if(isEmpty()){
		MT_LOG(LEV_WARN, "BlockQueue is NULL!\n");
		return;
	}
	int i = m_head;
	while(i != m_tail){
		MT_LOG(LEV_INFO, "%d\n", m_array[i]);
		i = (i + 1) % (m_max_size + 1);
	}
	MT_LOG(LEV_DEBUG, "Exit\n");
}

template<typename T>
oal_void blockQueue<T>::showArray(){
	MT_LOG(LEV_DEBUG, "Enter\n");
	MT_LOG(LEV_INFO, "m_head = %d, %d\n", m_head, getFront());
	MT_LOG(LEV_INFO, "m_tail = %d, %d\n", m_tail, getBack());
	MT_LOG(LEV_INFO, "m_size = %d , %d\n", m_size, getSize());
	MT_LOG(LEV_INFO, "m_max_size = %d\n", m_max_size);
	int i = 0;
	while(i != m_max_size + 1){
		MT_LOG(LEV_INFO, "%d\n", m_array[i]);
		i++;
	}
	MT_LOG(LEV_DEBUG, "Exit\n");
}
#endif