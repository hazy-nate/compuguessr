#ifndef CG_FASTCGI_H
#define CG_FASTCGI_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* core/cg_fastcgi.h, core/cg_fastcgi
 * NAME
 *   cg_fastcgi.h
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <linux/un.h>
#include <stdint.h>

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "platform/cg_env.h"
#include "sys/cg_types.h"

/*==============================================================================
 * DEFINITIONS
 *============================================================================*/

/* Version */
#define CG_FASTCGI_VERSION_1	1
#define CG_FASTCGI_VERSION	CG_FASTCGI_VERSION_1

/* Roles */
#define CG_FASTCGI_RESPONDER	0x0100
#define CG_FASTCGI_AUTHORIZER	0x0200
#define CG_FASTCGI_FILTER	0x0300

/* Flags */
#define CG_FASTCGI_KEEP_CONN          1

/* Protocol status codes */
#define CG_FASTCGI_REQUEST_COMPLETE	0x00000000
#define CG_FASTCGI_CANT_MPX_CONN	0x01000000
#define CG_FASTCGI_OVERLOADED		0x02000000
#define CG_FASTCGI_UNKNOWN_ROLE		0x03000000

/* Protocol sizes */
#define CG_FASTCGI_HEADER_SIZE		8
#define CG_FASTCGI_RECORD_MAX_SIZE	65790 /* 65535 + 255 */

/* Client sizes */
#define CG_FASTCGI_CLIENT_RX_BUF_SIZE	CG_FASTCGI_RECORD_MAX_SIZE
#define CG_FASTCGI_CLIENT_TX_BUF_SIZE	CG_FASTCGI_RECORD_MAX_SIZE

/*==============================================================================
 * ENUMERATIONS
 *============================================================================*/

enum cg_fastcgi_record_type {
	CG_FASTCGI_BEGIN_REQUEST	= 1,
	CG_FASTCGI_ABORT_REQUEST	= 2,
	CG_FASTCGI_END_REQUEST		= 3,
	CG_FASTCGI_PARAMS		= 4,
	CG_FASTCGI_STDIN		= 5,
	CG_FASTCGI_STDOUT		= 6,
	CG_FASTCGI_STDERR		= 7,
	CG_FASTCGI_DATA			= 8,
	CG_FASTCGI_GET_VALUES		= 9,
	CG_FASTCGI_GET_VALUES_RESULT	= 10,
	CG_FASTCGI_UNKNOWN_TYPE		= 11,
	CG_FASTCGI_MAXTYPE		= CG_FASTCGI_UNKNOWN_TYPE,
};

enum cg_fastcgi_state {
	CG_FASTCGI_STATE_ACCEPT_SQE = 0,
	CG_FASTCGI_STATE_ACCEPT_CQE,
	CG_FASTCGI_STATE_ERROR,
};

enum cg_fastcgi_status {
	CG_FASTCGI_STATUS_READY = 0,
};

enum cg_fastcgi_client_state {
	CG_FASTCGI_CLIENT_STATE_FREE = 0,
	CG_FASTCGI_CLIENT_STATE_READ_SQE,
	CG_FASTCGI_CLIENT_STATE_READ_CQE,
	CG_FASTCGI_CLIENT_STATE_WRITE_SQE,
	CG_FASTCGI_CLIENT_STATE_WRITE_CQE,
	CG_FASTCGI_CLIENT_STATE_CLOSE_SQE,
	CG_FASTCGI_CLIENT_STATE_CLOSE_CQE,
};

/*==============================================================================
 * STRUCTS
 *============================================================================*/

#pragma pack(push, 1)

struct cg_fastcgi_header {
	uint8_t version;
	uint8_t type;
	union {
		struct { uint8_t b1, b0; } b;
		uint16_t value;
	} request_id;
	union {
		struct { uint8_t b1, b0; } b;
		uint16_t value;
	} content_len;
	uint8_t padding_len;
	uint8_t reserved;
};

struct cg_fastcgi_begin_request_body {
	union {
		struct { uint8_t b1, b0; } b;
		uint16_t value;
	} role;
	uint8_t flags;
	uint8_t reserved[5];
};

struct cg_fastcgi_end_request_body {
	union {
		struct { uint8_t b3, b2, b1, b0; } b;
		uint32_t value;
	} app_status;
	uint8_t protocol_status;
	uint8_t reserved[3];
};

struct cg_fastcgi_unknown_type_body {
	uint8_t type;
	uint8_t reserved[7];
};

struct cg_fastcgi_nvp_11 {
	uint8_t name_len;
	uint8_t val_len;
};

struct cg_fastcgi_nvp_14 {
	uint8_t name_len;
	union {
		struct { uint8_t b3, b2, b1, b0; } b;
		uint32_t value;
	} val_len;
};

struct cg_fastcgi_nvp_41 {
	union {
		struct { uint8_t b3, b2, b1, b0; } b;
		uint32_t value;
	} name_len;
	uint8_t val_len;
};

struct cg_fastcgi_nvp_44 {
	union {
		struct { uint8_t b3, b2, b1, b0; } b;
		uint32_t value;
	} name_len;
	union {
		struct { uint8_t b3, b2, b1, b0; } b;
		uint32_t value;
	} val_len;
};

#pragma pack(pop)

struct cg_client_req;

struct cg_fastcgi_ctx {
	enum cg_fastcgi_state state;
	struct sockaddr_un sockaddr;
	int sockfd;
};

struct cg_fastcgi_client_ctx {
	enum cg_fastcgi_client_state state;
	struct cg_fastcgi_header saved_hdr;
	int sockfd;
	/* RX buffer */
	struct cg_buffer_metadata rx_buf_meta;
	char rx_buf[CG_FASTCGI_CLIENT_RX_BUF_SIZE];
	/* TX buffer */
	struct cg_buffer_metadata tx_buf_meta;
	char tx_buf[CG_FASTCGI_CLIENT_TX_BUF_SIZE];
};

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

void cg_fastcgi_client_init(struct cg_fastcgi_client_ctx *);
void cg_fastcgi_init(struct cg_env *, struct cg_fastcgi_ctx *);
int cg_fastcgi_parse(struct cg_buffer_metadata *, struct cg_client_req *, struct cg_fastcgi_header *);
void cg_fastcgi_stdout(struct cg_buffer_metadata *, uint16_t, char *, uint32_t);
void cg_fastcgi_end(struct cg_buffer_metadata *, uint16_t);

#endif /* !CG_FASTCGI_H */
