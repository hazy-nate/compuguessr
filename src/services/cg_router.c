/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* services/cg_router.c
 * NAME
 *   cg_router.c
 * FUNCTION
 *   API request routing and business logic. Implements all endpoint
 *   handlers, JSON serialization, and form-data parsing. [cite: 119-227]
 ******/

#include <sys/syscall.h>
#include <unistd.h>

#include "core/cg_client.h"
#include "data/cg_sessiondb.h"
#include "data/cg_userdb.h"
#include "data/cg_hashmap.h"
#include "data/cg_challengedb.h"
#include "services/cg_router.h"
#include "util/cg_util.h"

extern struct cg_sessiondb *g_sessiondb;
extern struct cg_userdb *g_userdb;

/****f* services/append_str
 * NAME
 *   append_str
 * SYNOPSIS
 *   static void append_str(char **p, const char *s)
 * FUNCTION
 *   Appends a null-terminated string to a target buffer and advances the pointer.
 * INPUTS
 *   * p - Pointer to the destination buffer pointer.
 *   * s - The source string to append.
 * SOURCE
 */
static void
append_str(char **p, const char *s)
{
	int len = cg_strlen_avx2((char *)s);
	cg_memcpy(*p, s, len);
	*p += len;
}
/******/

/****f* services/append_json_escaped
 * NAME
 *   append_json_escaped
 * SYNOPSIS
 *   static void append_json_escaped(char **p, const char *s)
 * FUNCTION
 *   Appends a string to a buffer while escaping characters for JSON compliance.
 * INPUTS
 *   * p - Pointer to the destination buffer pointer.
 *   * s - The source string containing potential special characters.
 * SOURCE
 */
static void
append_json_escaped(char **p, const char *s)
{
	while (*s) {
		switch (*s) {
			case '"':  *(*p)++ = '\\'; *(*p)++ = '"'; break;
			case '\\': *(*p)++ = '\\'; *(*p)++ = '\\'; break;
			case '\b': *(*p)++ = '\\'; *(*p)++ = 'b'; break;
			case '\f': *(*p)++ = '\\'; *(*p)++ = 'f'; break;
			case '\n': *(*p)++ = '\\'; *(*p)++ = 'n'; break;
			case '\r': *(*p)++ = '\\'; *(*p)++ = 'r'; break;
			case '\t': *(*p)++ = '\\'; *(*p)++ = 't'; break;
			default:
				if ((unsigned char)*s >= 0x20) {
					*(*p)++ = *s;
				}
				break;
		}
		s++;
	}
}
/******/

/****f* services/append_uint
 * NAME
 *   append_uint
 * SYNOPSIS
 *   static void append_uint(char **p, uint64_t val)
 * FUNCTION
 *   Converts an unsigned 64-bit integer to ASCII and appends it to a buffer.
 * INPUTS
 *   * p   - Pointer to the destination buffer pointer.
 *   * val - The 64-bit integer value.
 * SOURCE
 */
static void
append_uint(char **p, uint64_t val)
{
	char tmp[32];
	struct cg_ntoa_result res = cg_utoa(val, tmp);
	cg_memcpy(*p, (char *)res.ptr, res.len);
	*p += res.len;
}
/******/

/****f* services/cg_get_form_value
 * NAME
 *   cg_get_form_value
 * SYNOPSIS
 *   static int cg_get_form_value(const char *qs, const char *key, char *out)
 * FUNCTION
 *   Parses an application/x-www-form-urlencoded string to find a specific key value.
 * INPUTS
 *   * qs  - The raw query or STDIN string.
 *   * key - The form key to locate.
 *   * out - Buffer to store the decoded value.
 * RESULT
 *   * 1 if the key was found, 0 otherwise.
 * SOURCE
 */
static int
cg_get_form_value(const char *qs, const char *key, char *out)
{
	int key_len = cg_strlen_avx2((char *)key);
	const char *p = qs;
	while (*p) {
		int match = 1;
		for (int i = 0; i < key_len; i++) {
			if (p[i] != key[i]) {
				match = 0;
				break;
			}
		}
		if (match && p[key_len] == '=') {
			p += key_len + 1;
			char *dst = out;
			while (*p && *p != '&') {
				if (*p == '+') {
					*dst++ = ' ';
				} else if (*p == '%' && p[1] && p[2]) {
					int h = (p[1] <= '9') ? (p[1] - '0') : ((p[1] | 0x20) - 'a' + 10);
					int l = (p[2] <= '9') ? (p[2] - '0') : ((p[2] | 0x20) - 'a' + 10);
					*dst++ = (char)((h << 4) | l);
					p += 2;
				} else {
					*dst++ = *p;
				}
				p++;
			}
			*dst = '\0';
			return 1;
		}
		while (*p && *p != '&') p++;
		if (*p == '&') p++;
	}
	out[0] = '\0';
	return 0;
}
/******/

/****f* services/get_session_id
 * NAME
 *   get_session_id
 * SYNOPSIS
 *   static uint64_t get_session_id(struct cg_client_req *req)
 * FUNCTION
 *   Parses the numeric session ID from the request structure.
 * INPUTS
 *   * req - Pointer to the client request structure.
 * RESULT
 *   * The numeric representation of the session ID.
 * SOURCE
 */
static uint64_t
get_session_id(struct cg_client_req *req) {
	uint64_t sid = 0;
	for (int i = 0; req->session_id[i] != '\0'; i++) {
		if (req->session_id[i] >= '0' && req->session_id[i] <= '9') {
			sid = (sid * 10) + (req->session_id[i] - '0');
		} else break;
	}
	return sid;
}
/******/

/****f* services/cg_route_not_found
 * NAME
 *   cg_route_not_found
 * SYNOPSIS
 *   void cg_route_not_found(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Sends a 404 Not Found HTTP response to the client.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_not_found(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	char *body = "Status: 404 Not Found\r\n"
		     "Content-Type: text/plain\r\n\r\n"
		     "404 - Endpoint not configured.";
	uint32_t len = cg_strlen_avx2(body);
	cg_fastcgi_stdout(tx_meta, req->req_id, body, len);
}
/******/

/****f* services/cg_route_api_user_profile
 * NAME
 *   cg_route_api_user_profile
 * SYNOPSIS
 *   void cg_route_api_user_profile(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Handles GET requests to retrieve the authenticated user's profile details.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_user_profile(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;

	if (cg_memcmp_avx2(req->method, "GET", 3) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n{\"error\":\"405\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	if (req->session_id[0] == '\0') {
		char *err = "Status: 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"No session\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	uint64_t sid = get_session_id(req);
	struct cg_sessiondb_entry *entry = cg_sessiondb_entry_get(g_sessiondb, sid);
	if (!entry) {
		char *err = "Status: 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Invalid session\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char resp_buf[8192];
	char *p = resp_buf;
	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\n\r\n");
	if (entry->user_id == 0) {
		append_str(&p, "{\"status\":\"success\",\"username\":\"Guest\",\"is_guest\":true,\"total_points\":0,\"description\":\"\",\"completed_count\":0,\"completed_hashes\":[]}");
	} else {
		struct cg_userdb_entry *uentry = cg_userdb_find_by_id(g_userdb, entry->user_id);
		if (!uentry) {
			char *err = "Status: 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"User missing\"}";
			cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
			return;
		}
		append_str(&p, "{\"status\":\"success\",\"username\":\"");
		append_json_escaped(&p, uentry->username);
		append_str(&p, "\",\"is_guest\":false,\"total_points\":");
		append_uint(&p, uentry->total_points);
		append_str(&p, ",\"completed_count\":");
		append_uint(&p, uentry->completed_count);
		append_str(&p, ",\"description\":\"");
		append_json_escaped(&p, uentry->description);
		append_str(&p, "\",\"completed_hashes\":[");
		for (uint32_t i = 0; i < uentry->completed_count; i++) {
			if (i > 0) append_str(&p, ",");
			append_str(&p, "\"");
			append_uint(&p, uentry->completed_hashes[i]);
			append_str(&p, "\"");
		}
		append_str(&p, "]}");
	}

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_user_profile_update
 * NAME
 *   cg_route_api_user_profile_update
 * SYNOPSIS
 *   void cg_route_api_user_profile_update(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Handles POST requests to update user-specific data like descriptions.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_user_profile_update(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;

	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n{\"error\":\"405\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	uint64_t sid = get_session_id(req);
	struct cg_sessiondb_entry *entry = cg_sessiondb_entry_get(g_sessiondb, sid);
	if (!entry || entry->user_id == 0) {
		char *err = "Status: 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Unauthorized\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_userdb_entry *uentry = cg_userdb_find_by_id(g_userdb, entry->user_id);
	if (!uentry) {
		char *err = "Status: 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"User missing\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char description[2048];
	if (cg_get_form_value(req->stdin_buf, "description", description)) {
		int len = cg_strlen_avx2(description);
		if (len > 2047) len = 2047;
		cg_memcpy_avx2(uentry->description, description, len);
		uentry->description[len] = '\0';
	}

	char *resp = "Status: 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\"}";
	cg_fastcgi_stdout(tx_meta, req->req_id, resp, cg_strlen_avx2(resp));
}
/******/

/****f* services/cg_route_api_public_profile
 * NAME
 *   cg_route_api_public_profile
 * SYNOPSIS
 *   void cg_route_api_public_profile(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Handles POST requests to retrieve public information for a specific username.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_public_profile(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;

	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n{\"error\":\"405\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char username[64];
	if (!cg_get_form_value(req->stdin_buf, "username", username)) {
		char *err = "Status: 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Missing username\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_userdb_entry *uentry = cg_userdb_find(g_userdb, username);
	if (!uentry) {
		char *err = "Status: 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"User not found\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char resp_buf[8192];
	char *p = resp_buf;
	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\n\r\n");
	append_str(&p, "{\"status\":\"success\",\"username\":\"");
	append_json_escaped(&p, uentry->username);
	append_str(&p, "\",\"total_points\":");
	append_uint(&p, uentry->total_points);
	append_str(&p, ",\"completed_count\":");
	append_uint(&p, uentry->completed_count);
	append_str(&p, ",\"description\":\"");
	append_json_escaped(&p, uentry->description);
	append_str(&p, "\"}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_user_stats
 * NAME
 *   cg_route_api_user_stats
 * SYNOPSIS
 *   void cg_route_api_user_stats(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Retrieves and returns the point total for the currently logged-in user.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_user_stats(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;

	uint64_t sid = get_session_id(req);
	struct cg_sessiondb_entry *entry = cg_sessiondb_entry_get(g_sessiondb, sid);

	char resp_buf[256];
	char *p = resp_buf;
	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\n\r\n");
	if (!entry || entry->user_id == 0) {
		append_str(&p, "{\"points\":0}");
	} else {
		struct cg_userdb_entry *uentry = cg_userdb_find_by_id(g_userdb, entry->user_id);
		append_str(&p, "{\"points\":");
		append_uint(&p, uentry ? uentry->total_points : 0);
		append_str(&p, "}");
	}

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_user_delete
 * NAME
 *   cg_route_api_user_delete
 * SYNOPSIS
 *   void cg_route_api_user_delete(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Handles user account deletion and clears session cookies.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_user_delete(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;

	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\n\r\n";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	uint64_t sid = get_session_id(req);
	struct cg_sessiondb_entry *s_entry = cg_sessiondb_entry_get(g_sessiondb, sid);

	if (!s_entry || s_entry->user_id == 0) {
		char *err = "Status: 401 Unauthorized\r\n\r\n";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	cg_userdb_delete(g_userdb, s_entry->user_id);
	// Invalidate session in sessiondb if applicable, or just clear cookie

	char *resp = "Status: 200 OK\r\n"
		     "Content-Type: application/json\r\n"
		     "Set-Cookie: SESSION=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;\r\n\r\n"
		     "{\"status\":\"success\"}";
	cg_fastcgi_stdout(tx_meta, req->req_id, resp, cg_strlen_avx2(resp));
}
/******/

/****f* services/cg_route_api_leaderboard
 * NAME
 *   cg_route_api_leaderboard
 * SYNOPSIS
 *   void cg_route_api_leaderboard(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Calculates and returns the top 10 users by point total.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_leaderboard(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "GET", 3) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n{\"error\":\"405\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_userdb_entry *top[10] = {0};
	for (uint32_t i = 0; i < CG_USERDB_POOL_SIZE; i++) {
		if (g_userdb->pool[i].is_active) {
			struct cg_userdb_entry *curr = &g_userdb->pool[i];
			for (int j = 0; j < 10; j++) {
				if (!top[j] || curr->total_points > top[j]->total_points) {
					for (int k = 9; k > j; k--) {
						top[k] = top[k - 1];
					}
					top[j] = curr;
					break;
				}
			}
		}
	}

	char resp_buf[2048];
	char *p = resp_buf;
	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\n\r\n");
	append_str(&p, "{\"status\":\"success\",\"leaderboard\":[");
	int first = 1;
	for (int i = 0; i < 10; i++) {
		if (!top[i]) break;
		if (!first) append_str(&p, ",");
		append_str(&p, "{\"username\":\"");
		append_json_escaped(&p, top[i]->username);
		append_str(&p, "\",\"points\":");
		append_uint(&p, top[i]->total_points);
		append_str(&p, "}");
		first = 0;
	}

	append_str(&p, "]}");
	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_auth_guest
 * NAME
 *   cg_route_api_auth_guest
 * SYNOPSIS
 *   void cg_route_api_auth_guest(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Generates a guest session ID and sets the authentication cookie.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_auth_guest(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\n"
			    "Content-Type: application/json\r\n\r\n"
			    "{\"error\":\"405 - This endpoint requires a POST request.\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_sessiondb_entry entry;
	cg_sessiondb_entry_create(g_sessiondb, 0, &entry);

	char sid_str[32];
	struct cg_ntoa_result ntoa_res = cg_utoa(entry.session_id, sid_str);
	char resp_buf[512];
	char *p = resp_buf;

	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\nSet-Cookie: SESSION=");
	cg_memcpy(p, (char *)ntoa_res.ptr, ntoa_res.len);
	p += ntoa_res.len;
	append_str(&p, "; Path=/; SameSite=Lax\r\n\r\n{\"status\":\"success\",\"message\":\"Guest authenticated\"}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_auth_register
 * NAME
 *   cg_route_api_auth_register
 * SYNOPSIS
 *   void cg_route_api_auth_register(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Creates a new user account and authenticates the resulting user.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_auth_register(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n{\"error\":\"405\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char username[64];
	char password[64];
	if (!cg_get_form_value(req->stdin_buf, "username", username) || !cg_get_form_value(req->stdin_buf, "password", password)) {
		char *err = "Status: 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Missing credentials\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_userdb_entry uentry;
	if (cg_userdb_create(g_userdb, username, password, &uentry) != 0) {
		char *err = "Status: 409 Conflict\r\nContent-Type: application/json\r\n\r\n{\"error\":\"User exists or DB full\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_sessiondb_entry entry;
	cg_sessiondb_entry_create(g_sessiondb, uentry.id, &entry);

	char sid_str[32];
	struct cg_ntoa_result ntoa_res = cg_utoa(entry.session_id, sid_str);
	char resp_buf[512];
	char *p = resp_buf;

	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\nSet-Cookie: SESSION=");
	cg_memcpy(p, (char *)ntoa_res.ptr, ntoa_res.len);
	p += ntoa_res.len;
	append_str(&p, "; Path=/; SameSite=Lax\r\n\r\n{\"status\":\"success\",\"username\":\"");
	append_json_escaped(&p, uentry.username);
	append_str(&p, "\",\"total_points\":0}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_auth_login
 * NAME
 *   cg_route_api_auth_login
 * SYNOPSIS
 *   void cg_route_api_auth_login(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Verifies user credentials and establishes an authenticated session.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_auth_login(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n{\"error\":\"405\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char username[64];
	char password[64];
	if (!cg_get_form_value(req->stdin_buf, "username", username) || !cg_get_form_value(req->stdin_buf, "password", password)) {
		char *err = "Status: 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Missing credentials\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_userdb_entry *uentry = cg_userdb_find(g_userdb, username);
	uint64_t hash = cg_hash_str(password, cg_strlen_avx2(password));
	if (!uentry || uentry->pass_hash != hash) {
		char *err = "Status: 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Invalid credentials\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	struct cg_sessiondb_entry entry;
	cg_sessiondb_entry_create(g_sessiondb, uentry->id, &entry);

	char sid_str[32];
	struct cg_ntoa_result ntoa_res = cg_utoa(entry.session_id, sid_str);
	char resp_buf[512];
	char *p = resp_buf;
	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\nSet-Cookie: SESSION=");
	cg_memcpy(p, (char *)ntoa_res.ptr, ntoa_res.len);
	p += ntoa_res.len;
	append_str(&p, "; Path=/; SameSite=Lax\r\n\r\n{\"status\":\"success\",\"username\":\"");
	append_json_escaped(&p, uentry->username);
	append_str(&p, "\",\"total_points\":");
	append_uint(&p, uentry->total_points);
	append_str(&p, "}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_api_challenge_verify
 * NAME
 *   cg_route_api_challenge_verify
 * SYNOPSIS
 *   void cg_route_api_challenge_verify(struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Evaluates submitted answers for a challenge and updates user points.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 * SOURCE
 */
void
cg_route_api_challenge_verify(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\n"
			    "Content-Type: application/json\r\n\r\n"
			    "{\"error\":\"405 - This endpoint requires a POST request.\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	if (req->session_id[0] == '\0') {
		char *err = "Status: 401 Unauthorized\r\n"
			    "Content-Type: application/json\r\n\r\n"
			    "{\"error\":\"401 - Missing session.\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	uint64_t sid = get_session_id(req);
	struct cg_sessiondb_entry *entry = cg_sessiondb_entry_get(g_sessiondb, sid);
	if (!entry) {
		char *err = "Status: 401 Unauthorized\r\n"
			    "Content-Type: application/json\r\n\r\n"
			    "{\"error\":\"401 - Invalid session.\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	char chal_id[64];
	if (!cg_get_form_value(req->stdin_buf, "id", chal_id)) {
		char *err = "Status: 400 Bad Request\r\n"
			    "Content-Type: application/json\r\n\r\n"
			    "{\"error\":\"400 - Missing challenge ID.\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	const struct cg_challengedb *chal = cg_challengedb_find(chal_id);
	if (!chal) {
		char *err = "Status: 404 Not Found\r\n"
			    "Content-Type: application/json\r\n\r\n"
			    "{\"error\":\"404 - Challenge ID not found.\"}";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}

	uint32_t total_earned = 0;
	char resp_buf[4096];
	char *p = resp_buf;

	append_str(&p, "Status: 200 OK\r\nContent-Type: application/json\r\n\r\n");
	append_str(&p, "{\"status\":\"success\",\"message\":\"Payload evaluated.\",\"results\":[");

	const struct cg_question *questions = CG_RELOC(const struct cg_question *, chal->questions);
	for (uint32_t i = 0; i < chal->question_count; i++) {
		const struct cg_question *q = &questions[i];
		char q_key[16];
		char *q_key_p = q_key;
		append_str(&q_key_p, "q");
		append_uint(&q_key_p, q->id);
		*q_key_p = '\0';

		char submitted_val[256];
		int found = cg_get_form_value(req->stdin_buf, q_key, submitted_val);
		int is_correct = 0;

		if (found) {
			if (q->type == CG_ANS_TEXT) {
				const char *expected = CG_RELOC(const char *, q->answer.text_answer);
				if (cg_strcmp(submitted_val, expected) == 0) {
					is_correct = 1;
				}
			} else if (q->type == CG_ANS_BITMASK || q->type == CG_ANS_SINGLE) {
				uint32_t num_val = 0;
				for (int j = 0; submitted_val[j] != '\0'; j++) {
					if (submitted_val[j] >= '0' && submitted_val[j] <= '9') {
						num_val = (num_val * 10) + (submitted_val[j] - '0');
					}
				}
				if (num_val == q->answer.numeric_answer) {
					is_correct = 1;
				}
			}
		}

		if (is_correct) total_earned += q->points;

		if (i > 0) append_str(&p, ",");
		append_str(&p, "{\"id\":");
		append_uint(&p, q->id);
		append_str(&p, ",\"correct\":");
		append_str(&p, is_correct ? "true" : "false");
		append_str(&p, ",\"earned\":");
		append_uint(&p, is_correct ? q->points : 0);
		append_str(&p, ",\"explanation\":\"");
		const char *explanation = CG_RELOC(const char *, q->explanation);
		append_str(&p, explanation);
		append_str(&p, "\"}");
	}

	if (total_earned == chal->total_points && entry->user_id != 0) {
		struct cg_userdb_entry *uentry = cg_userdb_find_by_id(g_userdb, entry->user_id);
		if (uentry) {
			uentry->total_points += total_earned;
			uint64_t hash = cg_hash_str(chal_id, cg_strlen_avx2(chal_id));
			int recorded = 0;
			for (uint32_t i = 0; i < uentry->completed_count && i < 32; i++) {
				if (uentry->completed_hashes[i] == hash) {
					recorded = 1;
					break;
				}
			}
			if (!recorded && uentry->completed_count < 32) {
				uentry->completed_hashes[uentry->completed_count] = hash;
				uentry->completed_count++;
			}
		}
	}

	append_str(&p, "],\"total_earned\":");
	append_uint(&p, total_earned);
	append_str(&p, ",\"max_points\":");
	append_uint(&p, chal->total_points);
	append_str(&p, "}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}
/******/

/****f* services/cg_route_map_init
 * NAME
 *   cg_route_map_init
 * SYNOPSIS
 *   void cg_route_map_init(struct cg_hashmap *route_map)
 * FUNCTION
 *   Populates the routing hashmap with URI paths and handler pointers.
 * INPUTS
 *   * route_map - Pointer to the global route map.
 * SOURCE
 */
void
cg_route_map_init(struct cg_hashmap *route_map)
{
	union cg_ptr route_ptr;
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_auth_guest;
	cg_hashmap_insert_key_str(route_map, "/api/auth/guest", 15, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_auth_register;
	cg_hashmap_insert_key_str(route_map, "/api/auth/register", 18, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_auth_login;
	cg_hashmap_insert_key_str(route_map, "/api/auth/login", 15, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_user_profile;
	cg_hashmap_insert_key_str(route_map, "/api/user/profile", 17, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_user_profile_update;
	cg_hashmap_insert_key_str(route_map, "/api/user/profile/update", 24, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_public_profile;
	cg_hashmap_insert_key_str(route_map, "/api/user/public", 16, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_user_stats;
	cg_hashmap_insert_key_str(route_map, "/api/user/stats", 15, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_user_delete;
	cg_hashmap_insert_key_str(route_map, "/api/user/delete", 16, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_leaderboard;
	cg_hashmap_insert_key_str(route_map, "/api/leaderboard", 16, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_challenge_verify;
	cg_hashmap_insert_key_str(route_map, "/api/challenge/verify", 21, route_ptr);
}
/******/

/****f* services/cg_route_map_reply
 * NAME
 *   cg_route_map_reply
 * SYNOPSIS
 *   void cg_route_map_reply(struct cg_hashmap *route_map, struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Determines the appropriate route handler based on the URI and executes it.
 * INPUTS
 *   * route_map - Pointer to the global route map.
 *   * cli_ctx   - Pointer to the client context.
 * SOURCE
 */
void
cg_route_map_reply(struct cg_hashmap *route_map, struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	tx_meta->valid = 0;
	tx_meta->pos = 0;
	req->uri[CG_MAX_URI_LEN - 1] = '\0';

	struct cg_hashmap_entry *entry = cg_hashmap_get_key_str(route_map, req->uri, cg_strlen_avx2(req->uri));
	if (entry == 0) {
		cg_route_not_found(cli_ctx);
	} else {
		cg_route_func_t route_handler = (cg_route_func_t)entry->val.func_ptr;
		route_handler(cli_ctx);
	}

	cg_fastcgi_end(tx_meta, req->req_id);
}
/******/
