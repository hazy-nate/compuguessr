#ifndef CG_CHALLENGEDB_H
#define CG_CHALLENGEDB_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_challengedb.h
 * NAME
 *   cg_challengedb.h
 * FUNCTION
 *   Public interface for the challenge database lookup service.
 ******/

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "data/cg_challengedb_data.h"

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

const struct cg_challengedb *cg_challengedb_find(const char *);

#endif /* !CG_CHALLENGDB_H */
