#include "oslog.h"
#include <stdio.h>
void sysLog(char *buffer)
{
#ifdef _OS_LOG_ENABLE_
      //printf("%s\n",buffer);
	_lightOS_sysLogCallBack(buffer);
#endif
}
