#include"task.h"
#include"osmemory.h"
#include"oslog.h"
#include"watchdog.h"
#include"os_timer.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

static OS_TASK *os_taskList[OS_TASK_LIST_LENGTH];
static unsigned char os_taskStatus[OS_TASK_LIST_LENGTH];
static OS_TASK *current_running_task;
static unsigned int task_count;
static OS_TASK *taskNow;

void taskInit()
{
	taskNow = -1;
    task_count = 0;
    memset(os_taskList,0,sizeof(os_taskList));
    memset(os_taskStatus,0,sizeof(os_taskStatus));
    current_running_task = 0;
}

//add a task
OS_TASK *taskRegister(unsigned int (*funP)(int opt),unsigned long interval,unsigned char status,unsigned long temp_interval)
{
    OS_TASK *new_task;
	char log[50];
	//new_task = &(os_taskStatus[task_count]);
    if (funP)
    {
        // init task
        new_task = (OS_TASK *)osMalloc(sizeof(OS_TASK));
        new_task->taskP = funP;
        new_task->interval_time = interval;
        new_task->task_num = task_count;
        new_task->task_status = status;
        new_task->last_run_time = 0;
        new_task->temp_interval_time = temp_interval;

        // task add to list
        os_taskList[task_count] = new_task;
        os_taskStatus[task_count] = 1;
        task_count++;
#ifdef _OS_LOG_ENABLE_
		sprintf(log, "add task: %d\n", new_task->task_num);
		sysLog(log);
		sprintf(log, "task number: %d status : %d\n", new_task->task_num,new_task->task_status);
		sysLog(log);
#endif
        return new_task;
    }
    return 0;
}

void taskRestart(OS_TASK *task)
{
    task->task_status = 1;
}

void taskPause(OS_TASK *task)
{
    task->task_status = 0;
}

void taskNextDutyDelay(OS_TASK *task,long interval)
{
    task->temp_interval_time = interval;
}

void selfNextDutyDelay(long interval)
{
	taskNow->temp_interval_time = interval;
}

OS_TASK * taskSelfHandler()
{
	return current_running_task;
}


void os_taskProcessing()
{
#ifdef _OS_LOG_ENABLE_
    char log[50];
#endif
    unsigned int i;
    unsigned long interval = 0;
    unsigned long delta = 0;
    unsigned long time_now;
	unsigned int taskDutyIndicator = 0;
    OS_TASK *task;
    for (i = 0;i<task_count;i++)
    {
        // there is task
        if (os_taskStatus[i]==1)
        {
            task = os_taskList[i];
#ifdef _OS_LOG_ENABLE_
    		//sprintf(log,"Task %d status : %d\n",task->task_num,task->task_status);
        	//sysLog(log);
#endif
            // task status is running
            if (task->task_status == 1)
            {
                if (task->temp_interval_time >= 0)
                {
                    interval = task->temp_interval_time;
                }
                else
                {
                    interval =  task->interval_time;
                }

                time_now = getSysTime();
                if (time_now < task->last_run_time)
                {
                    delta = MAX_OS_TIMER_COUNT - (task->last_run_time - time_now);
                }
                else
                {
                    delta = time_now - task->last_run_time;
                }
                //4294967295
                //until interval time
                if (delta >= interval)
                {
                    task->temp_interval_time = -1;
                    current_running_task = task;
                    task->last_run_time = getSysTime();
#ifdef _OS_LOG_ENABLE_
                    //sprintf(log,"Running task: %d\n",task->task_num);
                    //sysLog(log);
#endif
					taskNow = task;
                    taskDutyIndicator += task->taskP(0);
#ifdef _WATCH_DOG_ENABLE_
                    watchDogFeed();
#endif
                }
            }
        }
    }
	if (taskDutyIndicator > 0) {
	
	}
	taskDutyIndicator = 0;
}





