#include <inc/lib.h>
#include <inc/pthread.h>

#define SEMKEY 5260
int a = 0;
void *countSub()
{
	int semid = semget(SEMKEY, 2, IPC_CREAT | 0666);
	int i = 0;
	for(i = 1; i <= 100; ++i){
		P(semid, 0);
		a += 1;
		cprintf(LIGHT_PURPLE"count says:%4d      \n"TAIL, a);
		V(semid, 1);
	}
	return NULL;
}
void *printSub()
{
	int semid = semget(SEMKEY, 2, IPC_CREAT | 0666);
	int i = 0;
	for(i = 1; i <= 100; ++i){
	//cprintf(RED"print waiting...\n"TAIL);
		//cprintf(LIGHT_CYAN"print waiting...\n"TAIL);
		P(semid, 1);
		cprintf(LIGHT_CYAN"print says:%4d\n"TAIL, a);
		V(semid, 0);
	}
	return NULL;
}

void 
umain(int argc, char **argv)
{
	cprintf("Hello World!\n");
	int semid;
	pthread_t p1, p2;
	/*创建信号量*/
	semid = semget(SEMKEY, 2, IPC_CREAT | 0666);
	if(semid == -1){
		cprintf("Error In semget:%e.\n", semid);
		return;
	}
	/*给信号量赋初值*/
	semctl(semid, 0, SETVAL, 1);//count
	semctl(semid, 1, SETVAL, 0);//print
	/*创建子线程*/
	pthread_create(&p1, NULL, countSub, NULL);
	pthread_create(&p2, NULL, printSub, NULL);
	/*等待子线程结束*/
	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	/*删除信号灯*/
	semctl(semid, 0, IPC_RMID, 0);
	//semctl(semid, 1, IPC_RMID);
	return;
}

