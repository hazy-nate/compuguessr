
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include "arch/cg_syscall.h"
#include "platform/cg_log.h"
#include "util/cg_util.h"

void
cg_log_write(const char *level, const char *file, const char *line, const char *msg)
{
	struct iovec iov[8];

	iov[0].iov_base = "(";
	iov[0].iov_len = 3;

	iov[1].iov_base = (void *)file;
	iov[1].iov_len = cg_strlen_avx2((char *)file);

	iov[2].iov_base = ".";
	iov[2].iov_len = 1;

	iov[3].iov_base = (void *)line;
	iov[3].iov_len = cg_strlen_avx2((char *)line);

	iov[4].iov_base = ") [";
	iov[4].iov_len = 3;

	iov[5].iov_base = (void *)level;
	iov[5].iov_len = cg_strlen_avx2((char *)level);

	iov[6].iov_base = "] ";
	iov[6].iov_len = 2;

	iov[7].iov_base = (void *)msg;
	iov[7].iov_len = cg_strlen_avx2((char *)msg);

	syscall3(SYS_writev, STDERR_FILENO, (long)iov, 8);
}

void
cg_log_write_ts(const char *datetime, const char *level, const char *file, const char *line, const char *msg)
{
	struct iovec iov[9];

	iov[0].iov_base = (void *)datetime;
	iov[0].iov_len = 19;

	iov[1].iov_base = ": (";
	iov[1].iov_len = 3;

	iov[2].iov_base = (void *)file;
	iov[2].iov_len = cg_strlen_avx2((char *)file);

	iov[3].iov_base = ".";
	iov[3].iov_len = 1;

	iov[4].iov_base = (void *)line;
	iov[4].iov_len = cg_strlen_avx2((char *)line);

	iov[5].iov_base = ") [";
	iov[5].iov_len = 3;

	iov[6].iov_base = (void *)level;
	iov[6].iov_len = cg_strlen_avx2((char *)level);

	iov[7].iov_base = "] ";
	iov[7].iov_len = 2;

	iov[8].iov_base = (void *)msg;
	iov[8].iov_len = cg_strlen_avx2((char *)msg);

	syscall3(SYS_writev, STDERR_FILENO, (long)iov, 9);
}
