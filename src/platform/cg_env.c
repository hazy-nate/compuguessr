/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* platform/cg_env.c
 * NAME
 *   cg_env.c
 * FUNCTION
 *   Environment and platform abstraction layer. Parses environment
 *   variables and resolves system functions via the vDSO. [cite: 250-268]
 ******/

#include <elf.h>
#include <sys/auxv.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include "arch/cg_syscall.h"
#include "platform/cg_env.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

long g_base_addr = 0;

/****f* platform/cg_env_init
 * NAME
 *   cg_env_init
 * SYNOPSIS
 *   __attribute__((weak)) void cg_env_init(char **envp, struct cg_env *env_out)
 * FUNCTION
 *   Initializes the environment configuration structure. It sets default
 *   values for the FastCGI socket path and then parses the provided
 *   environment pointer to find overrides. Additionally, it traverses
 *   the auxiliary vector to determine the program's base address via
 *   the AT_PHDR entry.
 * INPUTS
 *   * envp    - Pointer to the environment variable array.
 *   * env_out - Pointer to the cg_env structure to be populated.
 * SOURCE
 */
__attribute__((weak)) void
cg_env_init(char **envp, struct cg_env *env_out)
{
	if (!env_out) {
		return;
	}
	env_out->fastcgi_sock = CG_ENV_FASTCGI_SOCK_DEFAULT;
	env_out->fastcgi_sock_len = CG_ENV_FASTCGI_SOCK_DEFAULT_LEN;
	if (!envp) {
		return;
	}

	char **p = envp;
	while (*p != 0) {
		char *entry = *p;
		char *eq = entry;
		while (*eq && *eq != '=') {
			eq++;
		}
		if (*eq != '=') {
			p++;
			continue;
		}
		cg_size_t key_len = eq - entry;
		char *value = eq + 1;
		if (key_len == CG_ENV_FASTCGI_SOCK_KEY_LEN &&
		    cg_memcmp_avx2(entry, CG_ENV_FASTCGI_SOCK_KEY,
		    CG_ENV_FASTCGI_SOCK_KEY_LEN) == 0) {
			env_out->fastcgi_sock = value;
			env_out->fastcgi_sock_len = cg_strlen_avx2(value);
		}
		p++;
	}

	/* Skip the NULL terminator to access the auxiliary vector */
	p++;
	Elf64_auxv_t *auxv = (Elf64_auxv_t *)p;
	while (auxv->a_type != 0) {
		if (auxv->a_type == AT_PHDR) {
			/* Program headers are located immediately after the 64-byte ELF header */
			g_base_addr = (long)auxv->a_un.a_val - 64;
		}
		auxv++;
	}
}
/******/

/****f* platform/cg_env_write_stdout
 * NAME
 *   cg_env_write_stdout
 * SYNOPSIS
 *   __attribute__((weak)) void cg_env_write_stdout(struct cg_env *env)
 * FUNCTION
 *   Writes the current FastCGI socket configuration to standard output
 *   for logging purposes. It utilizes the writev syscall to perform
 *   a vectored write operation.
 * INPUTS
 *   * env - Pointer to the environment configuration structure.
 * SOURCE
 */
__attribute__((weak)) void
cg_env_write_stdout(struct cg_env *env)
{
	struct iovec iov[3];
	/*====================================================================*/
	iov[2].iov_base = (char *)"\n";
	iov[2].iov_len = 1;
	/*====================================================================*/
	iov[0].iov_base = "FASTCGI_SOCK = ";
	iov[0].iov_len = 15;
	iov[1].iov_base = env->fastcgi_sock;
	iov[1].iov_len = cg_strlen_avx2(env->fastcgi_sock);
	/*====================================================================*/
	syscall3(SYS_writev, STDOUT_FILENO, (long)iov, 3);
}
/******/

/****f* platform/cg_vdso_base_addr
 * NAME
 *   cg_vdso_base_addr
 * SYNOPSIS
 *   __attribute__((weak)) long cg_vdso_base_addr(char **envp)
 * FUNCTION
 *   Locates the base address of the vDSO (Virtual Dynamic Shared Object)
 *   by searching the auxiliary vector for the AT_SYSINFO_EHDR entry.
 * INPUTS
 *   * envp - Pointer to the environment variable array.
 * RESULT
 *   * The memory address of the vDSO header, or 0 if not found.
 * SOURCE
 */
__attribute__((weak)) long
cg_vdso_base_addr(char **envp)
{
	if (!envp) {
		return 0;
	}
	char **p = envp;
	while (*p != 0) {
		p++;
	}
	p++;
	Elf64_auxv_t *auxv = (Elf64_auxv_t *)p;
	long vdso_addr = 0;
	while (auxv->a_type != 0) {
		if (auxv->a_type == AT_SYSINFO_EHDR) {
			vdso_addr = (long)auxv->a_un.a_val;
			break;
		}
		auxv++;
	}
	return vdso_addr;
}
/******/

/****f* platform/cg_vdso_func
 * NAME
 *   cg_vdso_func
 * SYNOPSIS
 *   __attribute__((weak)) cg_generic_func_t cg_vdso_func(long vdso_base_addr, char *name)
 * FUNCTION
 *   Manually parses the ELF structures within the vDSO memory region
 *   to resolve a specific symbol by name. It traverses the program
 *   headers to find the dynamic section, and then searches the symbol
 *   table for a match.
 * INPUTS
 *   * vdso_base_addr - The base address where the vDSO is mapped.
 *   * name           - The name of the function to resolve.
 * RESULT
 *   * A pointer to the resolved function, or 0 if the symbol is not found.
 * SOURCE
 */
__attribute__((weak)) cg_generic_func_t
cg_vdso_func(long vdso_base_addr, char *name)
{
	union cg_ptr vdso_base = {
		.addr = vdso_base_addr
	};
	if (!vdso_base.ptr || !name) {
		return 0;
	}
	char *base = (char *)vdso_base.ptr;
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)__builtin_assume_aligned(base, 8);
	Elf64_Phdr *phdr = (Elf64_Phdr *)__builtin_assume_aligned(base + ehdr->e_phoff, 8);
	Elf64_Dyn *dyn = 0;
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type == PT_DYNAMIC) {
			dyn = (Elf64_Dyn *)__builtin_assume_aligned(base + phdr[i].p_offset, 8);
			break;
		}
	}
	if (!dyn) {
		return 0;
	}
	Elf64_Sym *symtab = 0;
	char *strtab = 0;
	for (int i = 0; dyn[i].d_tag != DT_NULL; i++) {
		switch (dyn[i].d_tag) {
		case DT_STRTAB:
			strtab = base + dyn[i].d_un.d_ptr;
			break;
		case DT_SYMTAB:
			symtab = (Elf64_Sym *)__builtin_assume_aligned(base + dyn[i].d_un.d_ptr, 8);
			break;
		default:
			break;
		}
	}
	if (!symtab || !strtab) {
		return 0;
	}
	int name_len = cg_strlen_avx2(name);
	for (int i = 0; i < 50; i++) {
		Elf64_Sym *sym = &symtab[i];
		if (sym->st_name == 0) {
			continue;
		}
		char *sym_name = strtab + sym->st_name;
		if (cg_memcmp_avx2(sym_name, name, name_len + 1) == 0) {
			union cg_ptr ret = {
				.addr = vdso_base_addr + (long)sym->st_value
			};
			return ret.func_ptr;
		}
	}
	return 0;
}
/******/
