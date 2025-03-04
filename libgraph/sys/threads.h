#ifndef SYS_THREADS_H
#define SYS_THREADS_H

typedef int handle_t;

int mutexCreate(handle_t *h);

int mutexLock(handle_t h);

int mutexUnlock(handle_t h);

int mutexTry(handle_t h);

int resourceDestroy(handle_t h);

#endif /* SYS_THREADS_H */