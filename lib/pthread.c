#include <inc/lib.h>

int pthread_create(pthread_t * thread, const pthread_attr_t * attr,  void * (*start_routine)(void*), void * arg) {
	*thread = sys_clone(start_routine, arg, NULL);
	return 0;
}