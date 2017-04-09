#ifndef WATCHDOG_H
#define	WATCHDOG_H

#ifdef	__cplusplus
extern "C" {
#endif
void watchDogInit(void);

void watchDogFeed(void);

void watchDogEnable(void);

void watchDogStop(void);



#ifdef	__cplusplus
}
#endif

#endif	/* WATCHDOG_H */

