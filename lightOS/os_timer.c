#include "os_timer.h"

unsigned long os_system_time;

void osTimerInit(void)
{
   os_system_time = 0;
}

unsigned long getSysTime(void)
{
    return os_system_time;
		//return HAL_GetTick();
}

// should be used in time int every 1ms.
void _system_time_auto_plus(void)
{
    os_system_time++;
}

