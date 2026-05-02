#ifndef CG_SYSCALL_H
#define CG_SYSCALL_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* arch/cg_syscall.h
 * NAME
 * cg_syscall.h
 * FUNCTION
 * Low-level system call wrappers for x86_64. Provides inline assembly
 * definitions for executing syscalls with 0 to 6 arguments.
 ******/

/*==============================================================================
 * INLINE FUNCTIONS
 *============================================================================*/

/****f* arch/syscall0
 * NAME
 *   syscall0
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall0(long n)
 * FUNCTION
 *   Executes a Linux system call with zero arguments.
 * INPUTS
 *   * n - The system call number (rax).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall0(long n)
{
	long ret;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

/****f* arch/syscall1
 * NAME
 *   syscall1
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall1(long n, long a1)
 * FUNCTION
 *   Executes a Linux system call with one argument.
 * INPUTS
 *   * n  - The system call number (rax).
 *   * a1 - The first argument (rdi).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall1(long n, long a1)
{
	long ret;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n), "D"(a1)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

/****f* arch/syscall2
 * NAME
 *   syscall2
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall2(long n, long a1, long a2)
 * FUNCTION
 *   Executes a Linux system call with two arguments.
 * INPUTS
 *   * n  - The system call number (rax).
 *   * a1 - The first argument (rdi).
 *   * a2 - The second argument (rsi).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall2(long n, long a1, long a2)
{
	long ret;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n), "D"(a1), "S"(a2)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

/****f* arch/syscall3
 * NAME
 *   syscall3
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall3(long n, long a1, long a2, long a3)
 * FUNCTION
 *   Executes a Linux system call with three arguments.
 * INPUTS
 *   * n  - The system call number (rax).
 *   * a1 - The first argument (rdi).
 *   * a2 - The second argument (rsi).
 *   * a3 - The third argument (rdx).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n), "D"(a1), "S"(a2), "d"(a3)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

/****f* arch/syscall4
 * NAME
 *   syscall4
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall4(long n, long a1, long a2, long a3, long a4)
 * FUNCTION
 *   Executes a Linux system call with four arguments.
 * INPUTS
 *   * n  - The system call number (rax).
 *   * a1 - The first argument (rdi).
 *   * a2 - The second argument (rsi).
 *   * a3 - The third argument (rdx).
 *   * a4 - The fourth argument (r10).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall4(long n, long a1, long a2, long a3, long a4)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

/****f* arch/syscall5
 * NAME
 *   syscall5
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5)
 * FUNCTION
 *   Executes a Linux system call with five arguments.
 * INPUTS
 *   * n  - The system call number (rax).
 *   * a1 - The first argument (rdi).
 *   * a2 - The second argument (rsi).
 *   * a3 - The third argument (rdx).
 *   * a4 - The fourth argument (r10).
 *   * a5 - The fifth argument (r8).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8  __asm__("r8")  = a5;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

/****f* arch/syscall6
 * NAME
 *   syscall6
 * SYNOPSIS
 *   __attribute__((always_inline, unused)) static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
 * FUNCTION
 *   Executes a Linux system call with six arguments.
 * INPUTS
 *   * n  - The system call number (rax).
 *   * a1 - The first argument (rdi).
 *   * a2 - The second argument (rsi).
 *   * a3 - The third argument (rdx).
 *   * a4 - The fourth argument (r10).
 *   * a5 - The fifth argument (r8).
 *   * a6 - The sixth argument (r9).
 * RESULT
 *   * The return value of the system call.
 * SOURCE
 */
__attribute__((always_inline, unused)) static inline long
syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8  __asm__("r8")  = a5;
	register long r9  __asm__("r9")  = a6;
	__asm__ volatile ("syscall"
	    : "=a"(ret)
	    : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
	    : "rcx", "r11", "memory");
	return ret;
}
/******/

#endif /* !CG_SYSCALL_H */
