/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* core/cg_fastcgi.c
 * NAME
 *   cg_fastcgi.c
 * FUNCTION
 *   FastCGI protocol state machine and record parser. Handles header
 *   processing, parameter extraction, and STDOUT/END_REQUEST generation.
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

/****f* cg_fastcgi/cg_fastcgi_init
 * NAME
 *   cg_fastcgi_init
 * SYNOPSIS
 *   void cg_fastcgi_init(struct cg_env *env, struct cg_fastcgi_ctx *ctx)
 * FUNCTION
 *   Initializes the FastCGI context with socket information. It
 *   zeroes the sockaddr structure, sets the family to AF_UNIX, and copies the
 *   socket path from the environment configuration .
 * INPUTS
 *   * env - Pointer to the environment configuration .
 *   * ctx - Pointer to the FastCGI context to initialize.
 * SOURCE
 */
void
cg_fastcgi_init(struct cg_env *env, struct cg_fastcgi_ctx *ctx)
{
	cg_memset_avx2(&ctx->sockaddr, 0, sizeof(ctx->sockaddr));
	ctx->sockaddr.sun_family = AF_UNIX;
	cg_memcpy_avx2(&ctx->sockaddr.sun_path, env->fastcgi_sock,
		       env->fastcgi_sock_len);
}
/******/

/****f* cg_fastcgi/cg_fastcgi_client_init
 * NAME
 *   cg_fastcgi_client_init
 * SYNOPSIS
 *   void cg_fastcgi_client_init(struct cg_fastcgi_client_ctx *ctx)
 * FUNCTION
 *   Initializes a client-specific FastCGI context . It sets the
 *   initial state to FREE, invalidates the socket descriptor, and configures
 *   the metadata for the internal RX and TX buffers .
 * INPUTS
 *   * ctx - Pointer to the FastCGI client context .
 * SOURCE
 */
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
/******/

/****f* cg_fastcgi/cg_fastcgi_len
 * NAME
 *   cg_fastcgi_len
 * SYNOPSIS
 *   static uint32_t cg_fastcgi_len(const uint8_t *buf, uint32_t *offset, uint32_t limit)
 * FUNCTION
 *   Decodes a FastCGI name-value pair length . If the first byte
 *   is less than 128, it returns that byte as the length.
 *   Otherwise, it treats the current byte and the following three bytes as a
 *   32-bit integer with the high bit masked out .
 * INPUTS
 *   * buf    - Pointer to the buffer containing encoded lengths.
 *   * offset - Pointer to the current read position, which is advanced .
 *   * limit  - The upper bound of the buffer to prevent over-reads .
 * RESULT
 *   * The decoded length as a 32-bit unsigned integer .
 * SOURCE
 */
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
/******/

/****f* cg_fastcgi/cg_fastcgi_handle_begin_request
 * NAME
 *   cg_fastcgi_handle_begin_request
 * SYNOPSIS
 *   static void cg_fastcgi_handle_begin_request(struct cg_client_req *req, struct cg_fastcgi_header *hdr, uint8_t *payload)
 * FUNCTION
 *   Processes an incoming FCGI_BEGIN_REQUEST record. It extracts
 *   the keep-alive flag and request ID, and resets the request state
 *   for a new transaction .
 * INPUTS
 *   * req     - Pointer to the client request structure to populate .
 *   * hdr     - Pointer to the FastCGI header for the record .
 *   * payload - Pointer to the record body.
 * SOURCE
 */
static void
cg_fastcgi_handle_begin_request(struct cg_client_req *req, struct cg_fastcgi_header *hdr, uint8_t *payload)
{
	struct cg_fastcgi_begin_request_body *body = (struct cg_fastcgi_begin_request_body *)payload;
	req->keep_alive = body->flags & CG_FASTCGI_KEEP_CONN;
	req->req_id = hdr->request_id.value;
	req->stdin_len = 0;
	req->stdin_buf[0] = '\0';
	req->session_id[0] = '\0';
}
/******/

/****f* cg_fastcgi/cg_fastcgi_handle_params
 * NAME
 *   cg_fastcgi_handle_params
 * SYNOPSIS
 *   static void cg_fastcgi_handle_params(struct cg_client_req *req, uint8_t *payload, uint16_t content_len)
 * FUNCTION
 *   Parses name-value pairs in an FCGI_PARAMS record . It
 *   specifically extracts REQUEST_URI, REQUEST_METHOD, and HTTP_COOKIE
 *   (extracting the SESSION cookie) .
 * INPUTS
 *   * req         - Pointer to the client request structure to populate.
 *   * payload     - Pointer to the record body containing name-value pairs.
 *   * content_len - Total length of the params payload .
 * SOURCE
 */
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
		} else if (n_len == 11 && cg_memcmp_avx2(name, "HTTP_COOKIE", 11) == 0) {
			char *cookie = (char *)val;
			for (uint32_t i = 0; i < v_len; i++) {
				if (v_len - i >= 8 && cg_memcmp_avx2(cookie + i, "SESSION=", 8) == 0) {
					uint32_t j = i + 8;
					while (j < v_len && cookie[j] != ';') {
						j++;
					}
					uint32_t clen = j - (i + 8);
					if (clen > 31) {
						clen = 31;
					}
					cg_memcpy_avx2(req->session_id, cookie + i + 8, (int)clen);
					req->session_id[clen] = '\0';
					break;
				}
			}
		}
	}
}
/******/

/****f* cg_fastcgi/cg_fastcgi_handle_stdin
 * NAME
 *   cg_fastcgi_handle_stdin
 * SYNOPSIS
 *   static int cg_fastcgi_handle_stdin(struct cg_client_req *req, uint16_t content_len, const uint8_t *payload)
 * FUNCTION
 *   Accumulates data from FCGI_STDIN records into the request buffer .
 *   If a zero-length record is received, it signals that the request
 *   body is fully received .
 * INPUTS
 *   * req         - Pointer to the client request structure.
 *   * content_len - Length of data in the current record .
 *   * payload     - Pointer to the data to be accumulated.
 * RESULT
 *   * CG_FASTCGI_STATUS_READY if the full request body is received.
 *   * 1 if more data is expected.
 * SOURCE
 */
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
/******/

/****f* cg_fastcgi/cg_fastcgi_parse
 * NAME
 *   cg_fastcgi_parse
 * SYNOPSIS
 *   int cg_fastcgi_parse(struct cg_buffer_metadata *meta, struct cg_client_req *req, struct cg_fastcgi_header *saved_hdr)
 * FUNCTION
 *   The primary FastCGI record parser . It processes raw bytes
 *   from the RX buffer, reconstructing headers and dispatching payloads
 *   to handlers based on record type .
 * INPUTS
 *   * meta      - Metadata for the RX buffer containing raw bytes.
 *   * req       - Pointer to the request structure to be populated.
 *   * saved_hdr - Pointer to a header used to maintain state between calls.
 * RESULT
 *   * CG_FASTCGI_STATUS_READY if a full transaction is parsed.
 *   * 1 if the buffer was processed but more data is required .
 * SOURCE
 */
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
/******/

/****f* cg_fastcgi/cg_fastcgi_stdout
 * NAME
 *   cg_fastcgi_stdout
 * SYNOPSIS
 *   void cg_fastcgi_stdout(struct cg_buffer_metadata *tx_meta, uint16_t req_id, char *http_resp, uint32_t resp_len)
 * FUNCTION
 *   Wraps an HTTP response payload into an FCGI_STDOUT record and
 *   writes it to the transmission buffer . It handles header
 *   construction, including request ID and content length .
 * INPUTS
 *   * tx_meta   - Metadata for the TX buffer.
 *   * req_id    - The request ID for the record .
 *   * http_resp - Pointer to the HTTP response string.
 *   * resp_len  - Length of the response string .
 * SOURCE
 */
void
cg_fastcgi_stdout(struct cg_buffer_metadata *tx_meta, uint16_t req_id, char *http_resp, uint32_t resp_len)
{
	if (tx_meta->valid + sizeof(struct cg_fastcgi_header) + resp_len > tx_meta->len) {
		return;
	}
	struct cg_fastcgi_header *hdr = (struct cg_fastcgi_header *)(tx_meta->buf + tx_meta->valid);
	hdr->version = CG_FASTCGI_VERSION;
	hdr->type = CG_FASTCGI_STDOUT;
	hdr->request_id.value = __builtin_bswap16(req_id); // FIXED
	hdr->content_len.value = __builtin_bswap16((uint16_t)resp_len);
	hdr->padding_len = 0;
	hdr->reserved = 0;
	tx_meta->valid += sizeof(struct cg_fastcgi_header);
	if (resp_len > 0) {
		cg_memcpy_avx2(tx_meta->buf + tx_meta->valid, http_resp, (int)resp_len);
		tx_meta->valid += resp_len;
	}
}
/******/

/****f* cg_fastcgi/cg_fastcgi_end
 * NAME
 *   cg_fastcgi_end
 * SYNOPSIS
 *   void cg_fastcgi_end(struct cg_buffer_metadata *tx_meta, uint16_t req_id)
 * FUNCTION
 *   Finalizes a FastCGI transaction . It writes an empty
 *   FCGI_STDOUT record followed by an FCGI_END_REQUEST record with
 *   completion status .
 * INPUTS
 *   * tx_meta - Metadata for the TX buffer.
 *   * req_id  - The request ID for the records .
 * SOURCE
 */
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
	hdr->request_id.value = __builtin_bswap16(req_id); // FIXED
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
/******/
