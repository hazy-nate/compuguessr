#include <sys/syscall.h>
#include <unistd.h>

#include "core/cg_client.h"
#include "data/cg_sessiondb.h"
#include "data/cg_hashmap.h"
#include "data/cg_challengedb.h"
#include "services/cg_router.h"
#include "util/cg_util.h"

extern struct cg_sessiondb *g_sessiondb;

static void
append_str(char **p, const char *s)
{
	int len = cg_strlen_avx2((char *)s);
	cg_memcpy(*p, s, len);
	*p += len;
}

static void
append_uint(char **p, uint64_t val)
{
	char tmp[32];
	struct cg_ntoa_result res = cg_utoa(val, tmp);
	// NOLINTNEXTLINE
	cg_memcpy(*p, (char *)res.ptr, res.len);
	*p += res.len;
}

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
					int h = (p[1] <= '9') ?
						(p[1] - '0') : ((p[1] | 0x20) - 'a' + 10);
					int l = (p[2] <= '9') ?
						(p[2] - '0') : ((p[2] | 0x20) - 'a' + 10);
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
	// NOLINTNEXTLINE
	cg_memcpy(p, (char *)ntoa_res.ptr, ntoa_res.len);
	p += ntoa_res.len;
	append_str(&p, "; Path=/\r\n\r\n{\"status\":\"success\",\"message\":\"Guest authenticated\"}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}

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

	uint64_t sid = 0;
	for (int i = 0; req->session_id[i] != '\0'; i++) {
		if (req->session_id[i] >= '0' && req->session_id[i] <= '9') {
			sid = (sid * 10) + (req->session_id[i] - '0');
		} else {
			break;
		}
	}

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

	// NOLINTNEXTLINE
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
				// NOLINTNEXTLINE
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
		// NOLINTNEXTLINE
		const char *explanation = CG_RELOC(const char *, q->explanation);
		append_str(&p, explanation);
		append_str(&p, "\"}");
	}

	append_str(&p, "],\"total_earned\":");
	append_uint(&p, total_earned);
	append_str(&p, ",\"max_points\":");
	append_uint(&p, chal->total_points);
	append_str(&p, "}");

	cg_fastcgi_stdout(tx_meta, req->req_id, resp_buf, (uint32_t)(p - resp_buf));
}

void
cg_route_map_init(struct cg_hashmap *route_map)
{
	union cg_ptr route_ptr;
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_auth_guest;
	cg_hashmap_insert_key_str(route_map, "/api/auth/guest", 15, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_challenge_verify;
	cg_hashmap_insert_key_str(route_map, "/api/challenge/verify", 21, route_ptr);
}

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
