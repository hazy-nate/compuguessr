#ifndef CG_URING_H
#define CG_URING_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* core/cg_uring.h
 * NAME
 *   cg_uring.h
 * FUNCTION
 *   Structures and context for the io_uring subsystem, managing
 *   submission and completion queue offsets and pointers.
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <linux/io_uring.h>
#include <stdint.h>

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "sys/cg_types.h"

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_uring_ctx {
	/* io_uring file descriptor */
	int fd;

	/* io_uring sizes */
	cg_size_t sq_sz;
	cg_size_t cq_sz;
	cg_size_t sqe_sz;

	/* io_uring params */
	struct io_uring_params params;

	/* Submission Queue (SQ) pointers */
	struct {
		union cg_ptr ring;
		union cg_ptr tail;
		union cg_ptr tail_local;
		union cg_ptr ring_mask;
		union cg_ptr flags;
		union cg_ptr array;
	} __attribute__((aligned(64))) sq;

	/* Completion Queue (CQ) pointers */
	struct {
		union cg_ptr ring;
		union cg_ptr head;
		union cg_ptr tail;
		union cg_ptr ring_mask;
		union cg_ptr cqes;
	} __attribute__((aligned(64))) cq;

	/* Submission Queue Entry (SQE) pointers */
	__attribute__((aligned(64)))
	union cg_ptr sqe;
	uint32_t sqe_tail;
};

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

int cg_uring_init(struct cg_uring_ctx *, int);
struct io_uring_sqe *cg_uring_sqe_get(struct cg_uring_ctx *);
int cg_uring_sq_enqueue(struct cg_uring_ctx *, struct io_uring_sqe *);
struct io_uring_cqe *cg_uring_cq_peek(struct cg_uring_ctx *);
int cg_uring_cqe_wait(struct cg_uring_ctx *, struct io_uring_cqe **);
void cg_uring_cq_discard(struct cg_uring_ctx *, int);

#endif /* !CG_URING_H */
