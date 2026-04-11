/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* <module>/<name>
 * NAME
 *   <name>
 * FUNCTION
 *   <function>
 ******/

#include <sys/syscall.h>
#include <unistd.h>
#define _SYS_SOCKET_H
#include <bits/socket.h>
#include <linux/un.h>
#include <stdint.h>

#include "core/cg_client.h"
#include "core/cg_fastcgi.h"
#include "platform/cg_env.h"
#include "util/cg_util.h"

void
cg_fastcgi_init(struct cg_env *env, struct cg_fastcgi_ctx *ctx)
{
	cg_memset_avx2(&ctx->sockaddr, 0, sizeof(ctx->sockaddr));
	ctx->sockaddr.sun_family = AF_UNIX;
	cg_memcpy_avx2(&ctx->sockaddr.sun_path, env->fastcgi_sock,
		       env->fastcgi_sock_len);
}

void
cg_fastcgi_client_init(struct cg_fastcgi_client_ctx *ctx)
{
	ctx->state = CG_FASTCGI_CLIENT_STATE_FREE;
	ctx->sockfd = -1;
	/* RX buffer */
	ctx->rx_buf_meta.buf = (uint8_t *)ctx->rx_buf;
	ctx->rx_buf_meta.len = CG_FASTCGI_CLIENT_RX_BUF_SIZE;
	ctx->rx_buf_meta.valid = 0;
	ctx->rx_buf_meta.pos = 0;
	/* TX buffer */
	ctx->tx_buf_meta.buf = (uint8_t *)ctx->tx_buf;
	ctx->tx_buf_meta.len = CG_FASTCGI_CLIENT_TX_BUF_SIZE;
	ctx->tx_buf_meta.valid = 0;
	ctx->tx_buf_meta.pos = 0;
}

static uint32_t
cg_fastcgi_len(const uint8_t *buf, uint32_t *offset, uint32_t limit)
{
	if (*offset >= limit) {
		return 0;
	}
	uint32_t len = buf[*offset];
	if (len < 128) {
		*offset += 1;
	} else {
		if (*offset + 4 > limit) return 0;
		len = ((uint32_t)(buf[*offset] & 0x7f) << 24) |
		      ((uint32_t)buf[*offset + 1] << 16) |
		      ((uint32_t)buf[*offset + 2] << 8) |
		      ((uint32_t)buf[*offset + 3]);
		*offset += 4;
	}
	return len;
}

static void
cg_fastcgi_handle_begin_request(struct cg_client_req *req, struct cg_fastcgi_header *hdr, uint8_t *payload)
{
	struct cg_fastcgi_begin_request_body *body = (struct cg_fastcgi_begin_request_body *)payload;
	req->keep_alive = body->flags & CG_FASTCGI_KEEP_CONN;
	req->req_id = hdr->request_id.value;
	req->stdin_len = 0;
	req->stdin_buf[0] = '\0';
}

static void
cg_fastcgi_handle_params(struct cg_client_req *req, uint8_t *payload, uint16_t content_len)
{
	uint32_t p_off = 0;
	while (p_off < content_len) {
		uint32_t n_len = cg_fastcgi_len(payload, &p_off, content_len);
		uint32_t v_len = cg_fastcgi_len(payload, &p_off, content_len);
		if (p_off + n_len + v_len > content_len) {
			break;
		}
		uint8_t *name = payload + p_off;
		p_off += n_len;
		uint8_t *val = payload + p_off;
		p_off += v_len;
		if (n_len == 11 && cg_memcmp_avx2(name, "REQUEST_URI", 11) == 0) {
			uint32_t clen = (v_len < 127) ? v_len : 127;
			cg_memcpy_avx2(req->uri, val, (int)clen);
			req->uri[clen] = '\0';
			for (uint32_t i = 0; i < clen; i++) {
				if (req->uri[i] == '?') {
					req->uri[i] = '\0';
					break;
				}
			}
		} else if (n_len == 14 && cg_memcmp_avx2(name, "REQUEST_METHOD", 14) == 0) {
			uint32_t clen = (v_len < 15) ? v_len : 15;
			cg_memcpy_avx2(req->method, val, (int)clen);
			req->method[clen] = '\0';
		}
	}
}

static int
cg_fastcgi_handle_stdin(struct cg_client_req *req, uint16_t content_len, const uint8_t *payload)
{
	if (content_len == 0) {
		return CG_FASTCGI_STATUS_READY;
	}
	if (req->stdin_len + content_len < CG_MAX_STDIN_LEN) {
		cg_memcpy(req->stdin_buf + req->stdin_len, (void *)payload, content_len);
		req->stdin_len += content_len;
		req->stdin_buf[req->stdin_len] = '\0';
	}
	return 1;
}

int
cg_fastcgi_parse(struct cg_buffer_metadata *meta, struct cg_client_req *req, struct cg_fastcgi_header *saved_hdr)
{
	while (meta->pos < meta->valid) {
		if (saved_hdr->type == 0) {
			if (meta->valid - meta->pos < sizeof(struct cg_fastcgi_header)) {
				return 1;
			}
			cg_memcpy_avx2(saved_hdr, meta->buf + meta->pos, sizeof(struct cg_fastcgi_header));
			meta->pos += sizeof(struct cg_fastcgi_header);
		}
		uint16_t content_len = __builtin_bswap16(saved_hdr->content_len.value);
		uint32_t total_payload = content_len + saved_hdr->padding_len;
		if (meta->valid - meta->pos < total_payload) {
			return 1;
		}
		uint8_t *payload = meta->buf + meta->pos;
		switch (saved_hdr->type) {
		case CG_FASTCGI_BEGIN_REQUEST:
			cg_fastcgi_handle_begin_request(req, saved_hdr, payload);
			break;
		case CG_FASTCGI_PARAMS:
			cg_fastcgi_handle_params(req, payload, content_len);
			break;
		case CG_FASTCGI_STDIN:
			if (cg_fastcgi_handle_stdin(req, content_len, payload) == CG_FASTCGI_STATUS_READY) {
				saved_hdr->type = 0;
				meta->pos += total_payload;
				return CG_FASTCGI_STATUS_READY;
			}
			break;
		default:
			break;
		}
		meta->pos += total_payload;
		saved_hdr->type = 0;
	}
	return 1;
}

void
cg_fastcgi_stdout(struct cg_buffer_metadata *tx_meta, uint16_t req_id, char *http_resp, uint32_t resp_len)
{
	if (tx_meta->valid + sizeof(struct cg_fastcgi_header) + resp_len > tx_meta->len) {
		return;
	}
	struct cg_fastcgi_header *hdr = (struct cg_fastcgi_header *)(tx_meta->buf + tx_meta->valid);
	hdr->version = CG_FASTCGI_VERSION;
	hdr->type = CG_FASTCGI_STDOUT;
	hdr->request_id.value = req_id;
	hdr->content_len.value = __builtin_bswap16((uint16_t)resp_len);
	hdr->padding_len = 0;
	hdr->reserved = 0;
	tx_meta->valid += sizeof(struct cg_fastcgi_header);
	if (resp_len > 0) {
		cg_memcpy_avx2(tx_meta->buf + tx_meta->valid, http_resp, (int)resp_len);
		tx_meta->valid += resp_len;
	}
}

void
cg_fastcgi_end(struct cg_buffer_metadata *tx_meta, uint16_t req_id)
{
	if (tx_meta->valid + sizeof(struct cg_fastcgi_header) + 8 > tx_meta->len) {
		return;
	}
	/* Empty STDOUT */
	cg_fastcgi_stdout(tx_meta, req_id, 0, 0);
	/* END_REQUEST Record */
	struct cg_fastcgi_header *hdr = (struct cg_fastcgi_header *)(tx_meta->buf + tx_meta->valid);
	hdr->version = CG_FASTCGI_VERSION;
	hdr->type = CG_FASTCGI_END_REQUEST;
	hdr->request_id.value = req_id;
	hdr->content_len.value = __builtin_bswap16(8);
	hdr->padding_len = 0;
	hdr->reserved = 0;
	tx_meta->valid += sizeof(struct cg_fastcgi_header);
	/*
	 * App Status = 0
	 * Protocol Status = 0 (CG_FASTCGI_REQUEST_COMPLETE)
	 */
	cg_memset_avx2(tx_meta->buf + tx_meta->valid, 0, 8);
	tx_meta->valid += 8;
}
