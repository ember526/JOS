// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		     : "=a" (ret)
		     : "i" (T_SYSCALL),
		       "a" (num),
		       "d" (a1),
		       "c" (a2),
		       "b" (a3),
		       "D" (a4),
		       "S" (a5)
		     : "cc", "memory");

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

int
sys_getcpuid(void)
{
	 return syscall(SYS_getcpuid, 0, 0, 0, 0, 0, 0);
}

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	return syscall(SYS_page_alloc, 1, envid, (uint32_t) va, perm, 0, 0);
}

int
sys_page_map(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva, int perm)
{
	return syscall(SYS_page_map, 1, srcenv, (uint32_t) srcva, dstenv, (uint32_t) dstva, perm);
}

int
sys_page_unmap(envid_t envid, void *va)
{
	return syscall(SYS_page_unmap, 1, envid, (uint32_t) va, 0, 0, 0);
}

// sys_exofork is inlined in lib.h

int
sys_env_set_status(envid_t envid, int status)
{
	return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0);
}

int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	return syscall(SYS_env_set_trapframe, 1, envid, (uint32_t) tf, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uint32_t) upcall, 0, 0, 0);
}

int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, int perm)
{
	return syscall(SYS_ipc_try_send, 0, envid, value, (uint32_t) srcva, perm, 0);
}

int
sys_ipc_recv(void *dstva)
{
	return syscall(SYS_ipc_recv, 1, (uint32_t)dstva, 0, 0, 0, 0);
}

int 
sys_clone(void* (*fcn)(void *), void *arg, void *stack)
{
	return syscall(SYS_clone, 0, (uint32_t)fcn, (uint32_t)arg, (uint32_t)stack, 0, 0);
}

int 
sys_thread_set_rtn_routine(pthread_t tid, void *rtn_routine)
{
	return syscall(SYS_thread_set_rtn_routine, 0, tid, (uint32_t)rtn_routine, 0, 0, 0);
}

void 
sys_thread_destroy()
{
	 syscall(SYS_thread_destroy, 0, 0, 0, 0, 0, 0);
}

int
sys_thread_join(pthread_t thread, void **value_ptr)
{
	return syscall(SYS_thread_join, 0, thread, (uint32_t)value_ptr, 0, 0, 0);
}

int
sys_thread_check_join(pthread_t thread)
{
	return syscall(SYS_thread_check_join, 0, thread, 0, 0, 0, 0);
}

int
sys_futex(pthread_mutex_t *uaddr, int op) 
{
	return syscall(SYS_futex, 0, (uint32_t)uaddr, op, 0, 0, 0);
}

int
sys_semget (uint32_t key, int nsem, int oflag)
{
	return syscall(SYS_semget, 0, key, nsem, oflag, 0, 0);
}

int
sys_semop (int semid, struct sembuf * opsptr, size_t nops)
{
	return syscall(SYS_semop, 0, semid, (uint32_t)opsptr, nops, 0, 0);
}

int
sys_semctl (int semid, int semnum, int cmd, int val )
{
	return syscall(SYS_semctl, 0, semid, semnum, cmd, val, 0);
}