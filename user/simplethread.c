#include <inc/lib.h>

void *mythread(void *arg) {
cprintf("hello from : %s\n", (char *) arg);
while (1);

return NULL;
}

void 
 umain(int argc, char **argv) {
 pthread_t p1, p2;
 int rc;
 cprintf("main: begin\n");
 rc = pthread_create(&p1, NULL, mythread, "A"); assert(rc == 0);
 rc = pthread_create(&p2, NULL, mythread, "B"); assert(rc == 0);
 // join waits for the threads to finish
 //rc = pthread_join(p1, NULL); assert(rc == 0);
 //rc = pthread_join(p2, NULL); assert(rc == 0);
 //printf("main: end\n");
 cprintf("main: looping\n");
while (1)
	;
 }