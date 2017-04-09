/*
 * File:   memory.h
 * Author: JI
 *
 * Created on 2014?10?22?, ??10:13
 */

#ifndef OS_MEMORY_H
#define	OS_MEMORY_H

#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif


void *osMalloc(unsigned int l);

void osMemRelease(void *p);



#ifdef	__cplusplus
}
#endif

#endif	/* MEMORY_H */

