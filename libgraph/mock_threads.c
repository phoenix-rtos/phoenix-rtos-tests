#include "sys/threads.h"
#include <string.h>

/* Global mock mutex for tracking state */
// static mutex_t mock_mutex = {0};

// mutex_t* mutex_get_mock_state(void) {
//     return &mock_mutex;
// }

// void mutex_reset_mock_state(void) {
//     memset(&mock_mutex, 0, sizeof(mock_mutex));
// }

// int mutexInit(handle_t m) {
//     // if (mutex != NULL) {
//     //     mutex->locked = 0;
//     //     mutex->lock_count = 0;
//     //     mutex->unlock_count = 0;
//     // }
//     return 0;
// }
int mutexCreate(handle_t *m) {
    return 0;
}

int mutexLock(handle_t m) {
    // mock_mutex.lock_count++;
    // mock_mutex.locked = 1;
    return 0;
}

int mutexUnlock(handle_t m) {
    // mock_mutex.unlock_count++;
    // mock_mutex.locked = 0;
    return 0;
}

int resourceDestroy(handle_t m) {
    return 0;
}