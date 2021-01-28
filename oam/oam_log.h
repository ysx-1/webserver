#ifndef __OAM_LOG_H
#define __OAM_LOG_H
/*
	文件说明：
	保存LOG相关 
*/
#include "../prepare.h"
enum LOG_LEVEL{
	LEV_OFF   = 0,
	LEV_ERROR = 1,
	LEV_WARN  = 2,
	LEV_INFO  = 3,
	LEV_DEBUG = 4,
	LEV_TOP   = 5,
};
oal_uint16 m_print_level = LEV_DEBUG;

#define MT_LOG(level, format, ...)\
	do\
	{\
		if(level <= m_print_level){\
			printf("[MT_DEBUG][%s,%s:%d]:" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}\
	}while(0)
/*单例模式，LOG类，用于LOG的保存文件处理*/
class LogMnmg{
public: 
	LogMnmg();
	~LogMnmg(){};
	oal_bool Init(oal_const oal_int8 *file_name, oal_int32 line_max, oal_int32 max_log_size, oal_int32 max_queue_size, oal_bool open = true);
	LogMnmg * GetInstance();
public:
	oal_static oal_const oal_int32 MAX_FILE_LEN = 128; 
private:
	oal_int64 m_count;/*当前文件中写入LOG的个数*/
	oal_bool m_open;/*记录当前日志保存是否打开，默认开启*/
	oal_bool m_async_flag;/*是否异步，默认同步*/
	oal_int64 m_day;/*记录当天的时间，用于更新LOG文件名*/
	oal_int32 m_line_max;/*每个文件中，最大LOG数目，由init函数初始化*/
	oal_int32 m_log_size_max;
	oal_int8 *m_buffer;
	oal_int8 m_dir_name[MAX_FILE_LEN];/*记录LOG文件目录，由init函数初始化*/
	oal_int8 m_file_name[MAX_FILE_LEN];/*记录LOG文件名，由init函数初始化*/
	oal_int8 m_file_full_name[ 2 * MAX_FILE_LEN ];
	FILE * m_fp;
};
LogMnmg::LogMnmg(){
	m_count = 0;
	m_open = true;
	m_async_flag = false;
}
/*懒汉式单例模式*/
LogMnmg *LogMnmg::GetInstance(){
	oal_static LogMnmg m_log_obj;
	return &m_log_obj;
}
oal_bool LogMnmg::Init(oal_const oal_int8 *file_name, oal_int32 line_max, oal_int32 max_log_size, oal_int32 max_queue_size, oal_bool open){
	MT_LOG(LEV_DEBUG, "Enter\n");
	time_t t;
	struct tm *sys_tm, my_tm;
	m_open = open;
	if(open == false){
		MT_LOG(LEV_INFO, "Save log is closed!\n");
		return true;
	}
	if(max_queue_size > 0){
		m_async_flag = true;
		//初始化阻塞队列

		//创建异步写文件线程
	}
	m_log_size_max = max_log_size;
	m_buffer = new oal_int8[max_log_size];
	m_line_max = line_max;

	/*获取当前时间*/
	t = time(NULL);
    sys_tm = localtime(&t);
    my_tm = *sys_tm;

	oal_const oal_int8* tmp = strrchr(file_name, '/');
	if(tmp == NULL){
		snprintf(m_file_full_name, 2 * MAX_FILE_LEN - 1,"%04d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
		strcpy(m_file_name, file_name);
	} else {
		strcpy(m_file_name, tmp + 1);
		strncpy(m_dir_name, file_name, tmp - file_name + 1);
		snprintf(m_file_full_name, 2 * MAX_FILE_LEN - 1,"%s%04d_%02d_%02d_%s", m_dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
	}
	m_day = my_tm.tm_mday;
    
    m_fp = fopen(m_file_full_name, "a");
    if (m_fp == NULL) {
		MT_LOG(LEV_ERROR, "fopen[%s]file failed!\n", m_file_full_name);
        return false;
    }
	MT_LOG(LEV_DEBUG, "Exit\n");
	return true;
}
oal_uint16 print_level = LEV_OFF;//LEV_DEBUG;
//#define print_level LEV_DEBUG
oal_uint16 save_level = LEV_DEBUG;
//#define save_level LEV_DEBUG

oal_inline oal_void save_log_function(oal_const oal_int8* format, ...);

#define LOG(level, format, ...)\
	do\
	{\
		if(level <= print_level){\
			printf("[SERVER_DEBUG][%s,%s:%d]:" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}\
		if(level <= save_level){\
			switch(level){\
				case LEV_ERROR: save_log_function("[ERROR]" format, ##__VA_ARGS__);break;\
				case LEV_WARN: save_log_function("[WARN]" format, ##__VA_ARGS__);break;\
				case LEV_INFO: save_log_function("[INFO]" format, ##__VA_ARGS__);break;\
				case LEV_DEBUG: save_log_function("[DEBUG]" format, ##__VA_ARGS__);break;\
				default:break;\
			};\
		}\
	}while(0)
	
#define LOG_ERRNO(_title)\
	do{\
		LOG(LEV_ERROR, "%s:%s\n", _title, strerror(errno));\
	}while(0)
/*perror 等价于 const char *s: strerror(errno) //提示符：发生系统错误的原因*/
#define LOG_ERRNO_CLEAN(_title) perror(_title);
/*
  errno 有效范围0-133 
  errno: 133      Memory page has hardware error
  errno: 134~255  unknown error!
*/


oal_void save_log_function(oal_const oal_int8* format, ...){
	oal_int8 buffer[256];
	va_list ptr;
	va_start(ptr, format);
	vsnprintf(buffer, sizeof(buffer) - 1, format, ptr);
	va_end(ptr);
	printf("%s", buffer);
}

#endif