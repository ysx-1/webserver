#ifndef __OAL_SLEEP_H
#define __OAL_SLEEP_H
#include "../prepare.h"

#define oal_sleep_s(_val) sleep(_val)
#define oal_sleep_ms(_val) usleep(1000 * _val)
#define oal_sleep_us(_val) usleep(_val)
#endif 