
#ifndef OS_CONFIG_H
#define	OS_CONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif

// system function shift
//#define _WATCH_DOG_ENABLE_  1
#define _OS_LOG_ENABLE_     1
//#define _OS_DEBUG_ON_

// timer
//#define OS_ST_PER_1_MS		1
//#define	OS_ST_PER_2_MS    OS_ST_PER_1_MS*2
#define	OS_ST_PER_5_MS    1
#define OS_ST_PER_10_MS    OS_ST_PER_5_MS*2
#define OS_ST_PER_20_MS    OS_ST_PER_10_MS*2
#define OS_ST_PER_50_MS    OS_ST_PER_10_MS*5
#define OS_ST_PER_100_MS    OS_ST_PER_10_MS*10
#define OS_ST_PER_SECOND   200
#define OS_ST_PER_MINUTE   12000
#define OS_ST_PER_30_MINUTE   360000

// system config
#ifndef OS_TASK_LIST_LENGTH
#define OS_TASK_LIST_LENGTH 10
#endif // !OS_TASK_LIST_LENGTH

#ifndef OS_EVENT_LIST_LENGTH
#define OS_EVENT_LIST_LENGTH 2
#endif // !OS_EVENT_LIST_LENGTH

#ifndef OS_HANDLER_LIST_LENGTH
#define OS_HANDLER_LIST_LENGTH  2
#endif // !OS_HANDLER_LIST_LENGTH

#define OS_HANDLER_LIST_LENGTH  2

// OS event
#define OS_EVT_SYSTEM_INIT  1


#ifdef	__cplusplus
}
#endif

#endif	/* OS_CONFIG_H */

