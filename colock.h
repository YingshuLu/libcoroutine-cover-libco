#ifndef CO_LOCK_H_
#define CO_LOCK_H_

typedef struct _co_lock colock_t;
    
void colock_init(colock_t *lock);
void colock_destory(colock_t *lock);
int co_lock(colock_t *lock);
int co_unlock(colock_t *lock); 

#endif
