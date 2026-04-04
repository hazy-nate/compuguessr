/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* sys/cg_uring.c
 * NAME
 *   cg_uring.c
 ******/

#include <linux/io_uring.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "sys/cg_syscall.h"
#include "sys/cg_uring.h"
#include "util/cg_util.h"

int
cg_uring_init(struct cg_uring_ctx *ctx, int flags)
{
	int ret = 0;

	cg_memset_avx2(&ctx->params, 0, sizeof(ctx->params));
	ctx->params.flags = flags;

	ctx->fd = (int)syscall2(SYS_io_uring_setup, 64, (long)&ctx->params);
	if (ctx->fd < 0) {
		return ctx->fd;
	}

	ctx->sq_sz = ctx->params.sq_off.array +
	    (ctx->params.sq_entries * sizeof(uint32_t));
	ctx->sqe_sz = ctx->params.sq_entries << 6;

	ctx->sq.ring.addr = syscall6(SYS_mmap, 0, (long)ctx->sq_sz,
	    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ctx->fd,
	    IORING_OFF_SQ_RING);
	if (ctx->sq.ring.ptr == MAP_FAILED) {
		ret = (int)ctx->sq.ring.addr;
		goto sq_mmap_error;
	}

	ctx->sq.tail.addr	= ctx->sq.ring.addr + ctx->params.sq_off.tail;
	ctx->sq.ring_mask.addr	= ctx->sq.ring.addr + ctx->params.sq_off.ring_mask;
	ctx->sq.flags.addr 	= ctx->sq.ring.addr + ctx->params.sq_off.flags;
	ctx->sq.array.addr 	= ctx->sq.ring.addr + ctx->params.sq_off.array;

	if (ctx->params.features & IORING_FEAT_SINGLE_MMAP) {
		ctx->cq.ring.addr = ctx->sq.ring.addr;
	} else {
		ctx->cq_sz = ctx->params.cq_off.cqes +
		    (ctx->params.cq_entries * sizeof(struct io_uring_cqe));
		ctx->cq.ring.addr = syscall6(SYS_mmap, 0, (long)ctx->cq_sz,
		    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ctx->fd,
		    IORING_OFF_CQ_RING);
		if (ctx->cq.ring.ptr == MAP_FAILED) {
			ret = (int)ctx->cq.ring.addr;
			goto cq_mmap_error;
		}
	}

	ctx->cq.head.addr	= ctx->cq.ring.addr + ctx->params.cq_off.head;
	ctx->cq.tail.addr	= ctx->cq.ring.addr + ctx->params.cq_off.tail;
	ctx->cq.ring_mask.addr	= ctx->cq.ring.addr + ctx->params.cq_off.ring_mask;
	ctx->cq.cqes.addr	= ctx->cq.ring.addr + ctx->params.cq_off.cqes;

	ctx->sqe.addr = syscall6(SYS_mmap, 0, (long)ctx->sqe_sz,
	    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ctx->fd,
	    IORING_OFF_SQES);
	if (ctx->sqe.ptr == MAP_FAILED) {
		ret = (int)ctx->sqe.addr;
		goto sqe_mmap_error;
	}

	return ret;
sqe_mmap_error:
	if (ctx->cq.ring.addr != ctx->sq.ring.addr) {
		syscall2(SYS_munmap, ctx->cq.ring.addr, (long)ctx->cq_sz);
	}
cq_mmap_error:
	syscall2(SYS_munmap, ctx->sq.ring.addr, (long)ctx->sq_sz);
sq_mmap_error:
	syscall1(SYS_close, ctx->fd);

	return ret;
}

struct io_uring_sqe *
cg_uring_sqe_init(struct cg_uring_ctx *ctx, int sqe_index)
{
	struct io_uring_sqe *sqe = &((struct io_uring_sqe *)ctx->sqe.ptr)[sqe_index];
	cg_memset_avx2(sqe, 0, sizeof(struct io_uring_sqe));

	return sqe;
}

int
cg_uring_sq_enqueue(struct cg_uring_ctx *ctx, int sqe_index)
{
	uint32_t tail		= *(uint32_t *)ctx->sq.tail.ptr;
	uint32_t ring_mask	= *(uint32_t *)ctx->sq.ring_mask.ptr;
	uint32_t *array_ptr	=  (uint32_t *)ctx->sq.array.ptr;

	array_ptr[tail & ring_mask] = sqe_index;

	__atomic_store_n((uint32_t *)ctx->sq.tail.ptr, tail + 1, __ATOMIC_RELEASE);

	uint32_t flags = *(uint32_t *)ctx->sq.flags.ptr;
	if (flags & IORING_SQ_NEED_WAKEUP) {
		syscall6(SYS_io_uring_enter, ctx->fd, 0, 0, IORING_ENTER_SQ_WAKEUP, 0, 0);
	}

	return 0;
}

struct io_uring_cqe *
cg_uring_cq_peek(struct cg_uring_ctx *ctx)
{
	uint32_t head = *(uint32_t *)ctx->cq.head.ptr;
	uint32_t tail = __atomic_load_n((uint32_t *)ctx->cq.tail.ptr, __ATOMIC_ACQUIRE);
	if (head == tail) {
		return (struct io_uring_cqe *)0;
	}

	uint32_t mask = *(uint32_t *)ctx->cq.ring_mask.ptr;
	struct io_uring_cqe *cqes = (struct io_uring_cqe *)ctx->cq.cqes.ptr;
	return &cqes[head & mask];
}

int
cg_uring_cqe_wait(struct cg_uring_ctx *ctx, struct io_uring_cqe **cqe_out)
{
	struct io_uring_cqe *cqe;
	cqe = cg_uring_cq_peek(ctx);
	if (cqe) {
		*cqe_out = cqe;
		return 0;
	}
	long ret = syscall6(SYS_io_uring_enter, (long)ctx->fd, 0, 1,
			    IORING_ENTER_GETEVENTS, 0, 0);
	if (ret < 0) {
		*cqe_out = 0;
		return (int)ret;
	}
	cqe = cg_uring_cq_peek(ctx);
	if (!cqe) {
		*cqe_out = 0;
		return -1;
	}
	*cqe_out = cqe;
	return 0;
}

void
cg_uring_cq_discard(struct cg_uring_ctx *ctx, int count)
{
	uint32_t head = *(uint32_t *)ctx->cq.head.ptr;
	__atomic_store_n((uint32_t *)ctx->cq.head.ptr, head + count, __ATOMIC_RELEASE);
}

