#ifndef __OAM_LOG_H
#define __OAM_LOG_H
/*
	文件说明：
	保存LOG相关 
*/
#include "../prepare.h"

/*单例模式，LOG类，用于LOG的保存文件处理*/
class LogMnmg{
public: 
	oal_bool Init(oal_const oal_int8 *file_name, oal_int32 line_max, oal_int32 max_log_size, oal_int32 max_queue_size, oal_bool open = true);
	oal_static LogMnmg * GetInstance();
	oal_void save_log_function(oal_const oal_int8* format, ...);
private:
	LogMnmg();
	~LogMnmg();
	oal_static oal_void* SaveLogToFileThread(oal_void *arg);
	oal_static oal_bool SaveLogToFile(oal_const oal_int8* onelog, oal_int32 sz);
public:
	oal_static oal_const oal_int32 MAX_FILE_LEN = 128; 
private:
	blockQueue<std::string> *m_blockQueue;
	oal_static oal_int64 m_count;/*当前文件中写入LOG的个数*/
	oal_bool m_open;/*记录当前日志保存是否打开，默认开启*/
	oal_bool m_async_flag;/*是否异步，默认同步*/
	oal_int64 m_day;/*记录当天的时间，用于更新LOG文件名*/
	oal_static oal_int32 m_line_max;/*每个文件中，最大LOG数目，由init函数初始化*/
	oal_int32 m_log_size_max;
	oal_int8 *m_buffer;
	oal_int8 m_dir_name[MAX_FILE_LEN];/*记录LOG文件目录，由init函数初始化*/
	oal_int8 m_file_name[MAX_FILE_LEN];/*记录LOG文件名，由init函数初始化*/
	oal_static oal_int8 m_file_full_name[ 2 * MAX_FILE_LEN ];
	oal_static FILE * m_fp;
};
/*静态成员的初始化*/
oal_int64 LogMnmg::m_count = 0;
oal_int32 LogMnmg::m_line_max = 0;
oal_int8 LogMnmg::m_file_full_name[ 2 * MAX_FILE_LEN ] = {0};
FILE * LogMnmg::m_fp = NULL;

LogMnmg::LogMnmg(){
	m_count = 0;
	m_open = true;
	m_async_flag = false;
}
LogMnmg::~LogMnmg(){
	m_count = 0;
	m_open = false;
	m_async_flag = true;
	if(m_blockQueue != NULL){
		delete m_blockQueue;
		m_blockQueue = NULL;
	}
}
/*懒汉式单例模式*/
LogMnmg *LogMnmg::GetInstance(){
	oal_static LogMnmg m_log_obj;
	return &m_log_obj;
}
oal_inline oal_void LogMnmg::save_log_function(oal_const oal_int8* format, ...){
	oal_int8 buffer[2048];
	va_list ptr;
	va_start(ptr, format);
	vsnprintf(buffer, sizeof(buffer) - 1, format, ptr);
	va_end(ptr);
	MT_LOG(LEV_DEBUG,"%s", buffer);
 	this->m_blockQueue->Push_m(std::string(buffer));
}
oal_void* LogMnmg::SaveLogToFileThread(oal_void *arg){
	MT_LOG(LEV_DEBUG, "Enter\n");
	blockQueue<std::string> *blockQue = (blockQueue<std::string> *)arg;
	bool m_exit_flag = false;
	std::string oneLog;
	while(!m_exit_flag){
		if(blockQue->Pop_m(oneLog)){
			MT_LOG(LEV_DEBUG,"%s", oneLog.c_str());
			SaveLogToFile(oneLog.c_str(), oneLog.size());
		}
	}
	//线程退出
	pthread_exit(0);
	MT_LOG(LEV_DEBUG, "Exit\n");
}
oal_bool LogMnmg::SaveLogToFile(oal_const oal_int8* onelog, oal_int32 sz){
	oal_bool ret = true;
	if(m_count > m_line_max){
		MT_LOG(LEV_WARN, "[%s] contain[%lld]lines, create a new file!",m_file_full_name, m_count);
		//return true;
		//新建文件，更新m_fp

	}
	ret = fwrite(onelog, 1, sz, m_fp);
	MT_LOG(LEV_DEBUG, "%d\n", ret);
	return ret;
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
		m_blockQueue = new blockQueue<std::string>(max_queue_size);

		//创建异步写文件线程。怎么安全结束线程！！？？？
		pthread_t write_log_tid;
		pthread_create(&write_log_tid, NULL, SaveLogToFileThread, (void*)m_blockQueue);
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
oal_uint16 print_level = LEV_DEBUG;//LEV_DEBUG;//LEV_DEBUG;
//#define print_level LEV_DEBUG
oal_uint16 save_level = LEV_DEBUG;//LEV_DEBUG;
//#define save_level LEV_DEBUG

oal_void save_log_function(oal_const oal_int8* format, ...);

#define LOG(level, format, ...)\
	do\
	{\
		if(level <= print_level){\
			printf("[SERVER_DEBUG][%s,%s:%d]:" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}\
		if(level <= save_level){\
			struct tm curTime, *pTime;\
			pTime = GetCutTime(&curTime);\
			switch(level){\
				case LEV_ERROR: \
				LogMnmg::GetInstance()->save_log_function("[%04d-%02d-%02d %02d:%02d:%02d][ERROR]" format,\
				pTime->tm_year ,pTime->tm_mon, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec, ##__VA_ARGS__); \
				break;\
				case LEV_WARN: \
				LogMnmg::GetInstance()->save_log_function("[%04d-%02d-%02d %02d:%02d:%02d][WARN]" format, \
				pTime->tm_year ,pTime->tm_mon, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec, ##__VA_ARGS__); \
				break;\
				case LEV_INFO: \
				LogMnmg::GetInstance()->save_log_function("[%04d-%02d-%02d %02d:%02d:%02d][INFO]" format, \
				pTime->tm_year ,pTime->tm_mon, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec, ##__VA_ARGS__); \
				break;\
				case LEV_DEBUG: \
				LogMnmg::GetInstance()->save_log_function("[%04d-%02d-%02d %02d:%02d:%02d][DEBUG]" format, \
				pTime->tm_year ,pTime->tm_mon, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec, ##__VA_ARGS__); \
				break;\
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
oal_inline oal_void save_log_function1(oal_const oal_int8* format, ...){
	oal_int8 buffer[256];
	va_list ptr;
	va_start(ptr, format);
	vsnprintf(buffer, sizeof(buffer) - 1, format, ptr);
	va_end(ptr);
	printf("%s", buffer);
}
#endif