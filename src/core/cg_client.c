#include <sys/syscall.h>
#include <unistd.h>

#include "core/cg_client.h"
#include "core/cg_fastcgi.h"

void
cg_client_pool_init(struct cg_client_ctx *cli_ctx_pool, int count)
{
	for (int i = 0; i < count; i++) {
		cli_ctx_pool[i].state = CG_CLIENT_STATE_FREE;
		cg_fastcgi_client_init(&cli_ctx_pool[i].fcgi_cli_ctx);
	}
}

struct cg_client_ctx *
cg_client_alloc(struct cg_client_ctx *cli_ctx_pool, int pool_size)
{
	for (int i = 0; i < pool_size; i++) {
		if (cli_ctx_pool[i].state == CG_CLIENT_STATE_FREE) {
			cli_ctx_pool[i].state = CG_CLIENT_STATE_IN_USE;
			return &cli_ctx_pool[i];
		}
	}
	return 0;
}

void
cg_client_free(struct cg_client_ctx *client)
{
	if (!client) {
		return;
	}
	client->state = CG_CLIENT_STATE_FREE;
}
