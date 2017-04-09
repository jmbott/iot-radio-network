/*
 * File:   task.h
 * Author: JI
 *
 * Created on 2014?10?21?, ??10:32
 */

#ifndef TASK_H
#define	TASK_H






#ifdef	__cplusplus
extern "C" {
#endif

#include "lightOS.h"
	
#define TASK_RUN  1
#define TASK_IDLE 0

void taskInit(void);
OS_TASK *taskRegister(unsigned int (*funP)(int opt),unsigned long interval,unsigned char status,unsigned long temp_interval);
void taskNextDutyDelay(OS_TASK *task,long interval);
void selfNextDutyDelay(long interval);
void taskRestart(OS_TASK *task);
void taskPause(OS_TASK *task);
OS_TASK * taskSelfHandler(void);
void os_taskProcessing(void);




#ifdef	__cplusplus
}
#endif

#endif	/* TASK_H */

