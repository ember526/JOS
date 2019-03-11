#ifndef JOS_INC_PTHREAD_H
#define JOS_INC_PTHREAD_H

typedef struct {
	int pad;
} pthread_attr_t;
typedef unsigned long int pthread_t;
typedef volatile uint32_t pthread_spinlock_t;

int pthread_create(pthread_t * thread, const pthread_attr_t * attr,  void * (*start_routine)(void*), void * arg);
void pthread_exit(void *rval_ptr);

int pthread_join(pthread_t thread, void **value_ptr);

int  pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int  pthread_spin_lock(pthread_spinlock_t *lock);
int  pthread_spin_unlock(pthread_spinlock_t *lock);

#endif /* !JOS_INC_PTHREAD_H */