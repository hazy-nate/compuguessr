#include <immintrin.h>
#include <stdint.h>

#include "core/cg_client.h"
#include "data/cg_challengedb.h"
#include "data/cg_hashmap.h"
#include "services/cg_router.h"
#include "util/cg_util.h"

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
                            "Content-Type: text/plain\r\n\r\n"
                            "405 - This endpoint requires a POST request.";
                cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
                return;
	}
	char *body = "Status: 200 OK\r\n"
		     "Content-Type: text/html\r\n\r\n"
		     "<html><body>"
		     "<h1>CompuGuessr Engine Active</h1>"
		     "<form action='/api/guess' method='POST'>"
		     "<input type='number' name='guess' placeholder='Enter a number'>"
		     "<button type='submit'>Submit</button>"
		     "</form></body></html>";
	uint32_t len = cg_strlen_avx2(body);
	cg_fastcgi_stdout(tx_meta, req->req_id, body, len);
}

void
cg_route_api_session(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\n"
                            "Content-Type: text/plain\r\n\r\n"
                            "405 - This endpoint requires a POST request.";
                cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
                return;
	}
	char *body = "Status: 200 OK\r\n"
		     "Content-Type: text/html\r\n\r\n"
		     "<html><body>"
		     "<h1>CompuGuessr Engine Active</h1>"
		     "<form action='/api/guess' method='POST'>"
		     "<input type='number' name='guess' placeholder='Enter a number'>"
		     "<button type='submit'>Submit</button>"
		     "</form></body></html>";
	uint32_t len = cg_strlen_avx2(body);
	cg_fastcgi_stdout(tx_meta, req->req_id, body, len);
}

void
cg_route_api_challenge(struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	struct cg_client_req *req = &cli_ctx->req;
	if (cg_memcmp_avx2(req->method, "POST", 4) != 0) {
		char *err = "Status: 405 Method Not Allowed\r\nContent-Type: text/plain\r\n\r\n405 - POST required.";
		cg_fastcgi_stdout(tx_meta, req->req_id, err, cg_strlen_avx2(err));
		return;
	}
}

void
cg_route_map_init(struct cg_hashmap *route_map)
{
	union cg_ptr route_ptr;
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_auth_guest;
	cg_hashmap_insert_key_str(route_map, "/api/auth/guest", 15, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_session;
	cg_hashmap_insert_key_str(route_map, "/api/session", 12, route_ptr);
	route_ptr.func_ptr = (cg_generic_func_t)cg_route_api_challenge;
	cg_hashmap_insert_key_str(route_map, "/api/challenge", 14, route_ptr);
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
