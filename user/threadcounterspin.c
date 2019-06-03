#include <inc/lib.h>
#include <inc/pthread.h>
static volatile int counter = 0;
pthread_spinlock_t spinlock;
//
// mythread()
//
// Simply adds 1 to counter repeatedly, in a loop
// No, this is not how you would add 10,000,000 to
// a counter, but it shows the problem nicely.
//
void *
mythread(void *arg)
{
	cprintf(LIGHT_PURPLE"%s: begin with"TAIL LIGHT_BLUE" SPIN LOCK\n"TAIL, (char *) arg);
	int i;
	for (i = 0; i < 1e6; i++) {
		pthread_spin_lock(&spinlock);
		counter = counter + 1;
		pthread_spin_unlock(&spinlock);
		//cprintf("%s: %d\n", arg, counter);
	}
	cprintf(LIGHT_GREEN"%s: done\n"TAIL, (char *) arg);
	return NULL;
}

//
// main()
//
// Just launches two threads (pthread_create)
// and then waits for them (pthread_join)
//
void
umain(int argc, char **argv)
{
	pthread_t p1, p2, p3, p4;
	cprintf(LIGHT_RED"main: begin (counter = %d)\n"TAIL, counter);
	pthread_create(&p1, NULL, mythread, "A");
	pthread_create(&p2, NULL, mythread, "B");
	pthread_create(&p3, NULL, mythread, "C");
	pthread_create(&p4, NULL, mythread, "D");
	// join waits for the threads to finish
	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	pthread_join(p3, NULL);
	pthread_join(p4, NULL);
	cprintf(LIGHT_RED"main: done (counter = %d)\n"TAIL, counter);
	return;
}
