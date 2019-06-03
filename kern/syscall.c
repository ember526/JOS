/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(&envs[ENVX(curenv->env_id)], s, len, PTE_P|PTE_U);
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

static int
sys_getcpuid(void)
{
	return cpunum();
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	struct Env *parent = curenv, *child = NULL;

	int r = env_alloc(&child, curenv->env_id);
	//cprintf("--------------------%d\n", r);
	if (r < 0)
		return r;
	child->env_status = ENV_NOT_RUNNABLE;
	child->env_tf = parent->env_tf;
	child->env_tf.tf_regs.reg_eax = 0;
	return child->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	struct Env *e = NULL;
	if (status!=ENV_RUNNABLE && status!=ENV_NOT_RUNNABLE)
		return -E_BAD_ENV;
	int r = envid2env(envid, &e, 1);
	if (r < 0)
		return r;
	e->env_status = status;
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env *e = NULL;
	int r = envid2env(envid, &e, 1);
	if (r < 0)
		return r;
	////user_mem_assert(e, tf, sizeof(struct Trapframe), PTE_P | PTE_W | PTE_U);
//
	////tf->tf_ds = GD_UD | 3;
	////tf->tf_es = GD_UD | 3;
	////tf->tf_ss = GD_UD | 3;
	tf->tf_eflags = FL_IF;
	tf->tf_cs = GD_UT | 3;
	////tf->tf_eflags |= FL_IOPL_MASK;
	e->env_tf = *tf;

	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env *e = NULL;
	int r = envid2env(envid, &e, 1);
	if (r < 0)
		return r;
	e->env_pgfault_upcall = func;
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	if ((uint32_t)va >= UTOP || (uint32_t)va%PGSIZE || !(perm&PTE_U) || !(perm&PTE_P) || (perm&~PTE_SYSCALL))
		return -E_INVAL;
	struct Env *e = NULL;
	int r = envid2env(envid, &e, 1);
	if (r < 0)
		return r;
	struct PageInfo *pp = page_alloc(ALLOC_ZERO);
	if(!pp)
		return -E_NO_MEM;
	r = page_insert(e->env_pgdir, pp, va, perm);
	if (r < 0)
		page_free(pp);
	return r;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	if ((uint32_t)srcva >= UTOP || (uint32_t)srcva%PGSIZE ||
		  (uint32_t)dstva >= UTOP || (uint32_t)dstva%PGSIZE ||
	    	!(perm&PTE_U) || !(perm&PTE_P) || (perm&~PTE_SYSCALL))
		return -E_INVAL;
	struct Env *srce = NULL, *dste = NULL;
	int r = 0;
	r = envid2env(srcenvid, &srce, 1);	if (r < 0)	return r;
	r = envid2env(dstenvid, &dste, 1);	if (r < 0)	return r;

	pte_t *ptep = NULL;
	struct PageInfo *pp = page_lookup(srce->env_pgdir, srcva, &ptep);
	//cprintf("-----srcva0x%x\n", srcva);
	if (!pp) 
		return -E_INVAL;
	if (perm&PTE_W)
		if(!(*ptep&PTE_W))
			return -E_INVAL;

	r = page_insert(dste->env_pgdir, pp, dstva, perm);
	return r;

}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	struct Env *e = NULL;
	int r = envid2env(envid, &e, 1);
	if (r < 0)
		return r;
	if ((uint32_t)va >= UTOP || (uint32_t)va%PGSIZE)
		return -E_INVAL;
	//pte_t *ptep = NULL;
	//struct PageInfo *pp = page_lookup(e->env_pgdir, va, &ptep);
	//if (ptep && (*ptep&PTE_P)) *ptep = 0;
	page_remove(e->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	struct Env *receiver = NULL;
	int r = envid2env(envid, &receiver, 0);
	if (r < 0)	return r;
	if (receiver->env_ipc_recving == 0)
		return -E_IPC_NOT_RECV;
	receiver->env_ipc_perm  = 0;
	if ((uint32_t)srcva < UTOP) {
		if ((uint32_t)srcva%PGSIZE 
		 	|| !(perm&PTE_U) || !(perm&PTE_P) || (perm&~PTE_SYSCALL))
				return -E_INVAL;
		pte_t *ptep = NULL;
		struct PageInfo *pp = page_lookup(curenv->env_pgdir, srcva, &ptep);
		if (!pp) return -E_INVAL;
		if ((perm&PTE_W) && !(*ptep&PTE_W))
			return -E_INVAL;
		r = page_insert(receiver->env_pgdir, pp, receiver->env_ipc_dstva, perm);
		if (r < 0)	{cprintf("666\n");return -E_INVAL;}
		receiver->env_ipc_perm  = perm;
	}
	
	receiver->env_ipc_recving = 0;
	receiver->env_ipc_value = value;
	receiver->env_ipc_from  = curenv->env_id;

	receiver->env_status = ENV_RUNNABLE;
	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	if ((uint32_t)dstva<UTOP && (uint32_t)dstva%PGSIZE)
		return -E_INVAL;
	//struct Env *e = NULL;
	//int r = envid2env(curenv->env_id, &e, 0);
	//if (r < 0)	return r;
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_tf.tf_regs.reg_eax = 0;
	sched_yield();
	return 0;
}

//uaddr:
//futex_op: operation on the futex
static int sys_futex(int *uaddr, int op) 
{//cprintf("inin uaddr = 0x%x\n", uaddr);
	struct Env *pivot = curenv;
	if (curenv->tid) { // if current env is no the main thread
		int r = envid2env(curenv->env_parent_id, &pivot, 0);
		assert(r==0);
	}
	int i ;
	for (i = 0; i < FUTEXARRAYLEN; ++i) {
		if (pivot->futex_array[i].uaddr == uaddr) {
			struct Env ** queue = pivot->futex_array[i].waiting_queue;
			switch (op) {
				case FUTEX_WAIT : 
					if(*uaddr == 0 ) {
						cprintf("**************\n");
						return 0;
					}
					for (int m = 0; m < FUTEXQUEUELEN; ++m) {
						//if(pivot->futex_array[i].waiting_queue[m] == curenv)
						//	return -E_INVAL;
						if(queue[m] == NULL) {
							++pivot->futex_array[i].waitingNM; 
							queue[m] = curenv;
							curenv->env_status = ENV_NOT_RUNNABLE;
							//sched_yield();
							return 0;
						}
					}
				case FUTEX_WAKEUP :
					if(*uaddr == 1 ) {
						cprintf("----------------\n");
						return 0;
					}
					for (int m = 0; m < FUTEXQUEUELEN; ++m) {

						if(queue[m] && queue[m]->env_status==ENV_NOT_RUNNABLE) {
							//if(--pivot->futex_array[i].waitingNM == 0)
							//	pivot->futex_array[i].uaddr = NULL;
							queue[m]->env_status = ENV_RUNNABLE;
							queue[m] = NULL;
							//sched_yield();
							return 0;
						}
					}

			}

		}
	}
	if (op == FUTEX_WAIT) {
		//cprintf("wait uaddr = 0x%x\n", uaddr);
		for (int i = 0; i < FUTEXARRAYLEN; ++i) {
			if (pivot->futex_array[i].uaddr == NULL) {
				pivot->futex_array[i].uaddr = uaddr;
				pivot->futex_array[i].waitingNM = 1;
				pivot->futex_array[i].waiting_queue[0] = curenv;
				curenv->env_status = ENV_NOT_RUNNABLE;
				//sched_yield();
				return 0;
			}

		}
	}
	//cprintf("pivot->futex_array[0].uaddr = 0x%x\n", pivot->futex_array[0].uaddr);
	//cprintf("pivot->futex_array[1].uaddr = 0x%x\n", pivot->futex_array[1].uaddr);
	//cprintf("pivot->futex_array[2].uaddr = 0x%x\n", pivot->futex_array[2].uaddr);
	//cprintf("wake uaddr = 0x%x\n", uaddr);
	//cprintf("    *uaddr = %d\n", *uaddr);
	//cprintf("i = %d\n", i);
	//cprintf("0x%d\n", op);
	//assert(0);
	//return -E_INVAL;
	return 0;
}



static int sys_thread_check_join(pthread_t tid)
{
	//for (int i = 1; i < THREADSNM; ++i) {
	//	if (curenv->join_array[i])
	//		return 0;
	//}
	//return 1;
	if(curenv->threads[tid])
		return 0;
	return 1;
}

static int sys_thread_join(pthread_t tid, void **value_ptr)
{
	//assert(curenv->threads[tid]);
	//curenv->join_array[tid] = 1;
	//while(curenv->threads[tid])
	//	;
	assert(0);
	return 0;
}

static void sys_thread_destroy()
{	//cprintf("\033[1;34m""destroying stack pos 0x%x\n""\033[m", (uint32_t)curenv->env_tf.tf_esp);
	struct Env *parent = NULL;
	int r = envid2env(curenv->env_parent_id, &parent, 0);
	if (r < 0) panic("parent invalid");
	//thread related resources
	parent->threads[curenv->tid] = NULL;
	assert(curenv->tid);
	memset((void *)THEADSTACKBTM(curenv->tid), 0, PGSIZE);
	r = sys_page_unmap(0, (void *)THEADSTACKBTM(curenv->tid));
	if (r < 0) panic("unmap invalid");
	parent->join_array[curenv->tid] = 0;
	//env related resources
	thread_env_destroy(curenv);
	//while(1) {cprintf("sdfsdfsd\n");}
	sys_yield();
}
//set the termination routine for threads
static int
sys_thread_set_rtn_routine(pthread_t tid, void *rtn_routine)
{
	struct Env *thread = curenv->threads[tid];
	//only the creator of the threat or itself can set the rtn routine
	if (thread == NULL) {
		cprintf(RED"failing thread id : %d\n"TAIL, thread);
		return -E_INVAL;
	}

	uint32_t *stackpos = (uint32_t *)(USTACKTOP - thread->tid*2*PGSIZE);
	stackpos -= 2;
	*stackpos = (uint32_t)rtn_routine;
	return 0;
}
static int
sys_clone(void* (*fcn)(void *), void *arg, void *stack)
{
	struct Env *parent = curenv, *child = NULL;
	//find a free thread slot for the new one
	//if there is no free thread, return -E_NO_MEM
	assert(parent->threads[0]);
	int i;
	for (i = 1; i < THREADSNM; ++i) {
		if (parent->threads[i] == NULL)
			break;
	}
	if(i == THREADSNM) {return -E_NO_MEM;}

	int r = env_alloc(&child, curenv->env_id);
	//cprintf("--------------------%d\n", r);
	if (r < 0)
		return r;
	child->env_status = ENV_NOT_RUNNABLE;
	child->env_tf = parent->env_tf;

	parent->threads[i] = child;
	child->tid = i;
	r = sys_page_alloc(0, (void *)(USTACKTOP-PGSIZE-child->tid*PGSIZE*2), PTE_W|PTE_P|PTE_U);
	if (r < 0) 	return r;
	uint32_t *stacktop = (uint32_t *)(USTACKTOP - child->tid*PGSIZE*2) - 1;
	//cprintf("\033[1;32m""------>new  thread stacktop 0x%x\n""\033[m", (uint32_t)stacktop);
	//cprintf("\033[1;32m""------>main thread stacktop 0x%x\n""\033[m", (uint32_t)parent->env_tf.tf_esp);
	//cprintf("\033[1;32m""------>new  thread ebp      0x%x\n""\033[m", (uint32_t)child ->env_tf.tf_regs.reg_ebp);
	//cprintf("\033[1;32m""------>main thread ebp      0x%x\n""\033[m", (uint32_t)parent->env_tf.tf_regs.reg_ebp);

	*stacktop = (uint32_t)arg;
	stacktop--;
	//*stacktop = (uint32_t)terminate;
	//cprintf("------>return value 0x%x\n", (uint32_t)terminate);

	child->env_tf.tf_esp = (uint32_t)stacktop;
	child->env_tf.tf_eip = (uint32_t)fcn;
	child->env_pgdir = parent->env_pgdir;
	child->env_status = ENV_RUNNABLE;
	return child->tid;

}

static int
sys_semget(uint32_t key, int nsem, int oflag)
{
	if (nsem > MAXSEMNM) return -E_INVAL;

	struct semid_ds *sem = NULL;
	int i = 0;
	for (; i < SEMSETNM; ++i) {
		if (semaphore[i].key == key) {
			sem = semaphore + i;
			break;
		}
	}
	if ((oflag & IPC_CREAT) && (oflag & IPC_EXCL) && sem)
		return -E_INVAL;
	if (sem)
		return i;

	//no sem matching key 
	if (!(oflag & IPC_CREAT))
		return -E_INVAL;
	//create a new one
	for (i = 0; i < SEMSETNM; ++i) {
		if (semaphore[i].key == 0) {
			sem = semaphore + i;
			break;
		}
	}
	if (!sem)
		return -E_NO_MEM;
	sem->sem_nsem = nsem;
	sem->key = key;
	sem->sempid = curenv->env_id;
	return i;
}

static int
sys_semop(int semid, struct sembuf * opsptr, size_t nops) {
	struct semid_ds *sem = &semaphore[semid];
	//cprintf ("%d %d %d", sem->key, sem->sempid, curenv->env_id);
	if (sem->key == 0 /*|| sem->sempid!=curenv->env_id*/) {
		cprintf("invalid sem\n");
		return -1;
	}
	for (int i = 0; i < nops; ++i) {

		struct sembuf *buf = &opsptr[i];
		if (buf->sem_num >= sem->sem_nsem){
			cprintf("wrong num\n");
			return -1;
		}
		unsigned short *semvalp = &(sem->semset[buf->sem_num].semval);
		short semop = buf->sem_op;
		if (semop < 0)
			//while (1) {
				//cprintf("0x%x %d %d\n", curenv->env_id, *semvalp, semop);
				if (*semvalp + semop >= 0) {
					*semvalp += semop;
				}
				else {
					*semvalp += semop;
					sys_futex((int *)semvalp, FUTEX_WAIT);
					*semvalp -= semop;
					//cprintf("wwwwwwwwwwwwww%d\n", *semvalp);
					return i + 1;
				}
			//}
		else {
			if (*semvalp <= 0) {
				//cprintf("hhhhhhhhhhhhhhh\n");
				sys_futex((int *)semvalp, FUTEX_WAKEUP);
			}
			//cprintf("hhhhhhhhhhhhhhh%d\n", *semvalp);
			*semvalp += semop;
			//cprintf("hhhhhhhhhhhhhhh%d\n", *semvalp);
		}

		//sem->semset[buf->sem_num] += buf->sem_op;
	}
	return 0;
}

static int
sys_semctl(int semid, int semnum, int cmd, int val ) {
	struct semid_ds *sem = &semaphore[semid];
	if (sem->key == 0 || sem->sempid!=curenv->env_id
					  || semnum >= sem->sem_nsem)
		return -1;

	if (cmd == SETVAL) {
		sem->semset[semnum].semval = val;
	}
	else if (cmd == IPC_RMID) {
		sem->sem_nsem = 0;
		sem->key = 0;
		sem->sempid = 0;
	}
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	//panic("###########%d\n", syscallno);
	switch (syscallno) {
		case SYS_cputs   	: sys_cputs ((const char*)a1, a2); break;
		case SYS_cgetc   	: return sys_cgetc   	 (); 
		case SYS_getenvid	: return sys_getenvid	 (); 
		case SYS_env_destroy: return sys_env_destroy(a1);
		case SYS_yield 		: sys_yield(); return 0;
		case SYS_exofork    : return 	sys_exofork();
		case SYS_env_set_status:return 	sys_env_set_status(a1, a2);
		case SYS_page_map   : return 	sys_page_map      (a1, (void*)a2, a3, (void *)a4, a5);
		case SYS_page_unmap : return 	sys_page_unmap    (a1, (void*)a2);
		case SYS_page_alloc : return 	sys_page_alloc    (a1, (void*)a2, a3);
		case SYS_env_set_pgfault_upcall : return sys_env_set_pgfault_upcall(a1, (void *)a2);
		case SYS_ipc_try_send : return sys_ipc_try_send(a1, a2, (void *)a3, a4);
		case SYS_ipc_recv     : return sys_ipc_recv((void *)a1);
		case SYS_env_set_trapframe : return sys_env_set_trapframe(a1, (struct Trapframe *)a2);
		case SYS_clone 		: return sys_clone((void *)a1, (void*)a2, (void*)a3);
		case SYS_thread_set_rtn_routine : return sys_thread_set_rtn_routine(a1, (void *)a2);
		case SYS_thread_destroy 		: sys_thread_destroy(); return 0;
		case SYS_thread_join : return sys_thread_join(a1, (void **)a2);
		case SYS_thread_check_join : return sys_thread_check_join(a1);
		case SYS_futex : return sys_futex((int *)a1, a2);
		case SYS_semget: return sys_semget (a1, a2, a3);
		case SYS_semop : return sys_semop  (a1, (struct sembuf *)a2, a3);
		case SYS_semctl: return sys_semctl (a1, a2, a3, a4);
		case SYS_getcpuid : return sys_getcpuid ();
		case NSYSCALLS		: assert(0);break;
	default:
		return -E_INVAL;
	}
	return 0;
}

