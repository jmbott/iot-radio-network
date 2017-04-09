#ifndef OS_TIMER_H
#define	OS_TIMER_H

#ifdef	__cplusplus
extern "C" {
#endif
	
#define MAX_OS_TIMER_COUNT 4294967295L  //unsigned long

void osTimerInit(void);
unsigned long getSysTime(void);
void _system_time_auto_plus(void);


#ifdef	__cplusplus
}
#endif

#endif	/* TIMER_H */

