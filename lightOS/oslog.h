#ifndef OSLOG_H
#define	OSLOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "lightOS.h"

void sysLog(char *data);
void _lightOS_sysLogCallBack(char *data);



#ifdef	__cplusplus
}
#endif



#endif	/* OSLOG_H */

