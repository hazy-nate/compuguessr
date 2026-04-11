#ifndef CG_CHALLENGEDB_DATA_H
#define CG_CHALLENGEDB_DATA_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_challengedb_data.h, data/cg_challengedb_data
 * NAME
 *   cg_challengedb_data.h
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <stdint.h>

/*==============================================================================
 * ENUMERATIONS
 *============================================================================*/

enum cg_answer_type {
        CG_ANS_TEXT,
        CG_ANS_BITMASK,
        CG_ANS_SINGLE
};

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_question {
        uint32_t id;
        uint32_t points;
        enum cg_answer_type type;
        union {
                const char *text_answer;
                uint32_t numeric_answer;
        } answer;
        const char *explanation;
};

struct cg_challengedb {
        const char *id_string;
        uint32_t total_points;
        uint32_t question_count;
        const struct cg_question *questions;
};

/*==============================================================================
 * EXTERNAL VARIABLES
 *============================================================================*/

extern const struct cg_challengedb g_challengedb[];
extern const uint32_t g_challengedb_size;

#endif /* CG_CHALLENGEDB_DATA_H */
