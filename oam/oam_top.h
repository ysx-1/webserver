#ifndef __OAM_TOP_H
#define __OAM_TOP_H
/*
	文件说明：
	OAM模块顶层头文件 
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

#define MT_LOG_ERRNO(_title)\
	do{\
		MT_LOG(LEV_ERROR, "%s:%s\n", _title, strerror(errno));\
	}while(0)
/*perror 等价于 const char *s: strerror(errno) //提示符：发生系统错误的原因*/
#define MT_LOG_ERRNO_CLEAN(_title) perror(_title);
/*
  errno 有效范围0-133 
  errno: 133      Memory page has hardware error
  errno: 134~255  unknown error!
*/

#endif