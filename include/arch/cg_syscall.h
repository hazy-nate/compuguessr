#ifndef CG_SYSCALL_H
#define CG_SYSCALL_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* arch/cg_syscall.h, arch/cg_syscall
 * NAME
 *   cg_syscall.h
 ******/

/*==============================================================================
 * INLINE FUNCTIONS
 *============================================================================*/

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

#endif /* !CG_SYSCALL_H */
