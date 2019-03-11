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