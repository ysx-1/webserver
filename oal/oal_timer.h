//
// Created by 郁帅鑫 on 2021/1/3.
//

#ifndef __OAL_TIMER_H
#define __OAL_TIMER_H
/*
	文件说明：
	Linux 用户态 定时器 定义
*/

class timer_unit;/*定时器链表单元前向声明*/
struct timer_context;/*定时器回调函数上下文的前向声明*/
typedef oal_void (*call_back_func) (timer_context*);/*timer回调函数宏定义*/

struct timer_context{
	oal_int32  m_fd;
	sockaddr_in m_address;
	timer_unit *m_timer;
};

class timer_unit{
public:
	timer_unit(
		time_t expire, 
		call_back_func cb_fun = NULL, 
		timer_context *context = NULL, 
		timer_unit *pre = NULL, 
		timer_unit *next = NULL
	):m_expire(expire), m_pre(pre), m_next(next), m_context(context), m_cb_fun(cb_fun){};
public:
	time_t m_expire;/*本timer单元的超时时间，这里使用绝对时间*/
	call_back_func m_cb_fun;
	timer_context *m_context;
	timer_unit *m_pre;
	timer_unit *m_next;
};

/*使用双向循环链表实现timer_list*/
class sort_timer_lst{
public:
	sort_timer_lst();
	~sort_timer_lst();
	oal_void insert_timer(time_t time_slot, call_back_func cb, timer_context *context);/*time_slot为时间间隔，单位S*/
	oal_void adjust_timer(timer_unit *timer, time_t time_slot);/*重新设置某个timer的到期时间，并调整在链表中的顺序*/
	oal_void erase_timer(timer_unit *timer);/*删除某个timer并释放内存*/
	oal_void tick();/*每次心跳，检查到期的timer，调用其callback后，删除，并释放内存*/
private:
	/*把已经初始化完成的timer结构提，插入到双向循环链表中*/
	oal_void m_insert_timer(timer_unit *timer);

	/*把链表里的某个timer从链表里拿出来*/
	oal_void m_erase_timer(timer_unit *timer);

	oal_bool is_empty();
private:
	timer_unit *m_head;
	//timer_unit *m_tail;
};


sort_timer_lst::sort_timer_lst(){
	m_head = new timer_unit(LONG_MAX);/*初始化timer_lst头，简化插入删除操作*/
	m_head->m_pre = m_head;
	m_head->m_next = m_head;
}

sort_timer_lst::~sort_timer_lst(){
	timer_unit *tmp = m_head->m_next;
	while(tmp != m_head){
		delete tmp;
		tmp = tmp->m_next;
	}
	delete m_head;
	m_head = NULL;
}

oal_void sort_timer_lst::insert_timer(time_t time_slot, call_back_func cb_fun, timer_context *context){
	LOG(LEV_DEBUG, "Enter\n");

	/*根据时间间隔计算绝对时间*/
	time_t expire = time_slot + time(NULL);
	context->m_timer = new timer_unit(expire, cb_fun, context);
	LOG(LEV_DEBUG, "Insert a timer [%ld]\n", expire);
	m_insert_timer(context->m_timer);

	LOG(LEV_DEBUG, "Exit\n");
}
oal_void sort_timer_lst::adjust_timer(timer_unit *timer, time_t time_slot){
	LOG(LEV_DEBUG, "Enter\n");

	/*根据时间间隔计算绝对时间*/
	time_t expire = time_slot + time(NULL);
	
	/*先从链表中移除*/
	m_erase_timer(timer);
	
	/*更新改timer的超时时间*/
	timer->m_expire = expire;
	
	/*放回链表*/
	m_insert_timer(timer);
}
oal_void sort_timer_lst::erase_timer(timer_unit *timer){
	LOG(LEV_DEBUG, "Enter\n");
	m_erase_timer(timer);
	if(timer != NULL){
		LOG(LEV_DEBUG, "timer is'n NULL\n");
		delete timer;
	}
	LOG(LEV_DEBUG, "Exit\n");
}

oal_void sort_timer_lst::tick(){
	LOG(LEV_DEBUG, "Enter\n");
	/*获取当前时间*/
	time_t cur_time = time(NULL);
	timer_unit *tmp = NULL;
	timer_unit *cur = m_head->m_next;
	if(is_empty()){
		LOG(LEV_DEBUG, "Timer list is NULL now!\n");
		goto Done;
	}
	while(cur != m_head){
		LOG(LEV_DEBUG,"cur -> expire = %ld,%ld!\n", cur->m_expire, cur_time);
		if(cur->m_expire >= cur_time)
			break;
		
		/*当前timer超时，调用其callback，然后删除该timer*/
		cur->m_cb_fun(cur->m_context);
		tmp = cur;
		LOG(LEV_DEBUG, "012\n");
		cur = cur->m_next;
		LOG(LEV_DEBUG, "0123\n");
		erase_timer(tmp);
	}
Done:
	LOG(LEV_DEBUG, "Exit\n");
	return;
}
oal_void sort_timer_lst::m_insert_timer(timer_unit *timer){
	LOG(LEV_DEBUG, "Enter\n");
	timer_unit *target_back = NULL;
	/*只有一个list头的情况也适用*/
	target_back = m_head->m_next;
	/*查找timer应该所处位置的下一节点*/
	while(timer->m_expire > target_back->m_expire){
		target_back = target_back->m_next;
	}
	timer->m_pre = target_back->m_pre;
	timer->m_next = target_back;

	timer->m_pre->m_next = timer;
	timer->m_next->m_pre = timer;

Done:
	LOG(LEV_DEBUG, "Exit\n");
	return;
}
oal_void sort_timer_lst::m_erase_timer(timer_unit *timer){
	LOG(LEV_DEBUG, "Enter\n");
	if(timer == NULL || timer == m_head) {
		LOG(LEV_WARN, "Timer is invalid!\n");
		goto Done;
	}
	if(is_empty()){
		LOG(LEV_WARN, "Timer list is NULL!\n");
		goto Done;
	}
	timer->m_next->m_pre = timer->m_pre;
	timer->m_pre->m_next = timer->m_next;
	
	timer->m_next = NULL;
	timer->m_pre = NULL;
Done:	
	LOG(LEV_DEBUG, "Exit\n");
	return;
}
oal_bool sort_timer_lst::is_empty(){
	return m_head->m_next == m_head && m_head->m_pre == m_head;
}

#ifdef _SORT_TIMER_LST_TEST

sort_timer_lst timer_list;
timer_context context;

oal_const oal_int32 test_slot = 2;

oal_void cb_test(timer_context *context){
	LOG(LEV_DEBUG, "This timer user's fd is %d\n", context->m_fd++);
}

oal_void sort_timer_lst_test(){
	LOG(LEV_DEBUG, "Enter\n");
	context.m_fd = 1;
	timer_list.insert_timer(test_slot, cb_test, &context);

	timer_list.insert_timer(test_slot, cb_test, &context);

	timer_list.insert_timer(test_slot, cb_test, &context);

	timer_list.insert_timer(2 * test_slot, cb_test, &context);

	LOG(LEV_DEBUG, "Exit\n");
}

#endif

#endif //__OAL_TIMER_H
