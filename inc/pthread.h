#ifndef JOS_INC_PTHREAD_H
#define JOS_INC_PTHREAD_H

typedef struct {
	//int pad;
} pthread_attr_t;

typedef struct {
	
} pthread_mutexattr_t;

//enum {
//	FUTEX_WAIT = 1,
//	FUTEX_WAKEUP
//};

typedef unsigned long int pthread_t;

typedef volatile uint32_t pthread_spinlock_t;
typedef volatile int32_t pthread_mutex_t;

int pthread_create(pthread_t * thread, const pthread_attr_t * attr,  void * (*start_routine)(void*), void * arg);
void pthread_exit(void *rval_ptr);

int pthread_join(pthread_t thread, void **value_ptr);

int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int semget(uint32_t key, int nsem, int oflag);
int semop(int semid, struct sembuf * opsptr, size_t nops);
int semctl(int semid, int semnum, int cmd, int val);
void P(int semid, int index);
void V(int semid, int index);

#endif /* !JOS_INC_PTHREAD_H */