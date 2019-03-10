#ifndef JOS_INC_PTHREAD_H
#define JOS_INC_PTHREAD_H

typedef struct {
	int pad;
} pthread_attr_t;
typedef unsigned long int pthread_t;


int pthread_create(pthread_t * thread, const pthread_attr_t * attr,  void * (*start_routine)(void*), void * arg);
void pthread_exit(void *rval_ptr);

int pthread_join(pthread_t thread, void **value_ptr);
#endif /* !JOS_INC_PTHREAD_H */