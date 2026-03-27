
#include <immintrin.h>

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
