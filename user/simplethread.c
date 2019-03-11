#include <inc/lib.h>

void *mythread(void *arg) {
cprintf(LIGHT_PURPLE"[0x%.8x] hello from : %s\n"TAIL, sys_getenvid(), (char *) arg);

//cprintf(LIGHT_PURPLE" 0x%xhello from %s\n"TAIL, sys_getenvid(), arg);
return NULL;
}

void 
 umain(int argc, char **argv)
 {
 pthread_t p1, p2, p3, p4, p5, p6;
 int rc;
 cprintf("main: begin\n");
 cprintf("[0x%8x] hello from : %s\n", sys_getenvid(), "main");
 rc = pthread_create(&p1, NULL, mythread, "A"); assert(rc == 0);
 rc = pthread_create(&p2, NULL, mythread, "B"); assert(rc == 0);
 rc = pthread_create(&p3, NULL, mythread, "C"); assert(rc == 0);
 rc = pthread_create(&p4, NULL, mythread, "D"); assert(rc == 0);
//rc = pthread_create(&p5, NULL, mythread, "E"); assert(rc == 0);
//rc = pthread_create(&p6, NULL, mythread, "F"); assert(rc == 0);
 // join waits for the threads to finish
 rc = pthread_join(p1, NULL); assert(rc == 0);
 rc = pthread_join(p2, NULL); assert(rc == 0);
 rc = pthread_join(p3, NULL); assert(rc == 0);
 rc = pthread_join(p4, NULL); assert(rc == 0);
 cprintf("main: ending\n");
//while (1)
//	;
 }