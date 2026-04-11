
#include <elf.h>
#include <immintrin.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "arch/cg_syscall.h"
#include "util/cg_util.h"

__attribute__((weak)) char *
cg_get_env(char **envp, const char *target_key_str)
{
	if (!envp || !target_key_str) {
		return 0;
	}
	for (int i = 0; envp[i] != 0; i++) {
		const char *env_str = envp[i];
		const char *key_str = target_key_str;
		while (*key_str && *env_str == *key_str) {
			key_str++;
			env_str++;
		}
		if (*key_str == '\0' && *env_str == '=') {
			return (char *)(env_str + 1);
		}
	}
	return 0;
}

__attribute__((weak)) void *
cg_memcpy(void *dest, const void *src, unsigned long n)
{
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	while (n--) {
		*d++ = *s++;
	}
	return dest;
}

__attribute__((target("avx2"), weak)) int
cg_memcmp_avx2(const void *s1, const void *s2, int n)
{
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;
	while (n >= 32) {
		__m256i v1 = _mm256_loadu_si256((const void *)p1);
		__m256i v2 = _mm256_loadu_si256((const void *)p2);
		__m256i cmp = _mm256_cmpeq_epi8(v1, v2);
		unsigned int mask = _mm256_movemask_epi8(cmp);
		if (mask != 0xFFFFFFFF) {
			unsigned int mismatch_mask = ~mask;
			int diff_idx = __builtin_ctz(mismatch_mask);
			return (int)(p1[diff_idx] - p2[diff_idx]);
		}
		p1 += 32;
		p2 += 32;
		n -= 32;
	}
	while (n--) {
		if (*p1 != *p2) {
			return (int)(*p1 - *p2);
		}
		p1++;
		p2++;
	}
	return 0;
}

__attribute__((weak)) void
cg_memset(void *dest, int val, unsigned long n)
{
	for (unsigned long i = 0; i < n; i++) {
		*((char *)dest + i) = (char)val;
	}
}

__attribute__((weak)) char *
cg_memslide(char *s1, char *s2, int n)
{
	long diff = s1 + n - s2;
	cg_memcpy_avx2(s1, s2, (int)(diff));
	return s1 + diff;
}

__attribute__((weak)) int
cg_strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

__attribute__((target("sse4.2"))) uint64_t
cg_hash_str(const char *str, uint64_t len) {
	uint64_t crc = 0xFFFFFFFF;
	const char *p = str;
	while (len >= 8) {
		uint64_t val;
		__builtin_memcpy(&val, p, 8);
		crc = _mm_crc32_u64(crc, val);
		p += 8;
		len -= 8;
	}
	while (len > 0) {
		crc = _mm_crc32_u8((uint32_t)crc, (uint8_t)*p);
		p++;
		len--;
	}
	return crc;
}

void
cg_puts(char *str)
{
	int len = cg_strlen_avx2(str);
	char new_str[len + 2];
	cg_memcpy(new_str, str, len);
	new_str[len] = '\n';
	new_str[len + 1] = 0;
	syscall3(SYS_write, STDOUT_FILENO, (long)new_str, len + 1);
}
