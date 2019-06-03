/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ENV_H
#define JOS_INC_ENV_H

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

typedef int32_t envid_t;

// An environment ID 'envid_t' has three parts:
//
// +1+---------------21-----------------+--------10--------+
// |0|          Uniqueifier             |   Environment    |
// | |                                  |      Index       |
// +------------------------------------+------------------+
//                                       \--- ENVX(eid) --/
//
// The environment index ENVX(eid) equals the environment's offset in the
// 'envs[]' array.  The uniqueifier distinguishes environments that were
// created at different times, but share the same environment index.
//
// All real environments are greater than 0 (so the sign bit is zero).
// envid_ts less than 0 signify errors.  The envid_t == 0 is special, and
// stands for the current environment.

#define LOG2NENV		10
#define NENV			(1 << LOG2NENV)
#define ENVX(envid)		((envid) & (NENV - 1))
//for threads
#define THREADSNM 		5
#define THEADSTACKBTM(tid)   (USTACKTOP - PGSIZE * (tid*2+1))
#define THEADSTACKTOP(tid)   (USTACKTOP - PGSIZE * (tid*2  ))
//for futex
#define FUTEXARRAYLEN 5
#define FUTEXQUEUELEN 5

enum {
	FUTEX_WAIT = 1,
	FUTEX_WAKEUP
};
//for semaphore
#define SEMSETNM  256
#define MAXSEMNM  10
#define IPC_CREAT 0x200
#define IPC_EXCL  0x400

// Values of env_status in struct Env
enum {
	ENV_FREE = 0,
	ENV_DYING,
	ENV_RUNNABLE,
	ENV_RUNNING,
	ENV_NOT_RUNNABLE
};

// Special environment types
enum EnvType {
	ENV_TYPE_USER = 0,
	ENV_TYPE_FS,		// File system server
};

struct futex_mapping {
	int *uaddr;
	int waitingNM;
	struct Env * waiting_queue[FUTEXQUEUELEN];
};

struct Env {
	struct Trapframe env_tf;	// Saved registers
	struct Env *env_link;		// Next free Env
	envid_t env_id;			// Unique environment identifier
	envid_t env_parent_id;		// env_id of this env's parent
	enum EnvType env_type;		// Indicates special system environments
	unsigned env_status;		// Status of the environment
	uint32_t env_runs;		// Number of times environment has run
	int env_cpunum;			// The CPU that the env is running on

	// Address space
	pde_t *env_pgdir;		// Kernel virtual address of page dir

	// Exception handling
	void *env_pgfault_upcall;	// Page fault upcall entry point

	// Lab 4 IPC
	bool env_ipc_recving;		// Env is blocked receiving
	void *env_ipc_dstva;		// VA at which to map received page
	uint32_t env_ipc_value;		// Data value sent to us
	envid_t env_ipc_from;		// envid of the sender
	int env_ipc_perm;		// Perm of page mapping received

	//for threads
	//int threads_nm;
	int tid;
	struct Env *threads[THREADSNM];
	int join_array[THREADSNM];
	//for futex
	struct futex_mapping futex_array[FUTEXARRAYLEN];

};


struct semid_ds {
	unsigned short sem_nsem; /* #number of semaphores in set, zero if the set is free*/
	uint32_t key; /*to which key the semaphore binds*/
	envid_t sempid; /*to which environment the semaphore set belongs*/
	struct {
		unsigned short semval; 	/* semaphore value, always >= 0 */
		//unsigned short semncnt; /*not used : # processes awaiting semval>curval */
		//unsigned short semzcnt; /*not used : # processes awaiting semval==0 */
		//envid_t sempid; 			/*not used : pid for last operation */
		int tid;
	} semset[MAXSEMNM];
};

struct sembuf {
	unsigned short sem_num;  /* semaphore number */
	short          sem_op;   /* semaphore operation */
	short          sem_flg;  /* operation flags */
};

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};

enum {
	SETVAL,
	IPC_RMID
};

extern struct semid_ds semaphore[SEMSETNM];

#endif // !JOS_INC_ENV_H
