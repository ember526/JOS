#include <inc/lib.h>
#include <inc/x86.h>
int pthread_create(pthread_t * thread, const pthread_attr_t * attr,  void * (*start_routine)(void*), void * arg)
{
	int r =  sys_clone(start_routine, arg, NULL);
	if (r < 0)
		return r;
	*thread = r;
	// the following line causes page fault
	//cprintf(RED"thread id : %d\n"TAIL, *thread);
	assert(0 == sys_thread_set_rtn_routine(*thread, pthread_exit));
	return 0;
}

void pthread_exit(void *rval_ptr)
{
	sys_thread_destroy();
	//while(1);
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	//int r = sys_thread_join(thread, value_ptr);
	while(sys_thread_check_join(thread)==0){
		;//cprintf(GREEN"waiting\n"TAIL);
	}
	return 0;
}

int  pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
	*lock = 0;
	return 0;
}

int  pthread_spin_lock(pthread_spinlock_t *lock)
{
	while (xchg(lock, 1) != 0)
		asm volatile ("pause");
	return 0;
}

int  pthread_spin_unlock(pthread_spinlock_t *lock)
{
	xchg(lock, 0);
	return 0;
}

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
	*mutex = 1;
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	fetch_and_add(mutex, -1);
	if (*mutex != 0)
		sys_futex(mutex, FUTEX_WAIT);
	return 0;

}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	fetch_and_add(mutex, 1);
	if (*mutex != 1)
		sys_futex(mutex, FUTEX_WAKEUP);
	return 0;
}

int semget(uint32_t key, int nsem, int oflag)
{
	return sys_semget(key, nsem, oflag);
}

int semop(int semid, struct sembuf * opsptr, size_t nops)
{
	int r = 1;
	struct sembuf *tmp = opsptr;
	do {
		nops = nops - r + 1;
		r = sys_semop(semid, tmp+r-1, nops);
	} while (r);
	sys_semop(semid, opsptr, nops);
	//cprintf (GREEN"%d\n"TAIL, r);
	return r;
}

int semctl(int semid, int semnum, int cmd, int val)
{
	return sys_semctl(semid, semnum, cmd, val);
}

void P(int semid, int index)
{
	struct sembuf sem;
	sem.sem_num = index;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	assert(semop(semid, &sem, 1) == 0);
	//cprintf (GREEN"%d\n"TAIL, 153);
}
void V(int semid, int index)
{
	struct sembuf sem;
	sem.sem_num = index;
	sem.sem_op = 1;
	sem.sem_flg = 0;
	assert(semop(semid, &sem, 1) == 0);
}
