#include <string.h>

#include "data/cg_challengedb_data.h"
#include "data/cg_challengedb.h"
#include "util/cg_util.h"

const struct cg_challengedb *
cg_challengedb_find(const char *id)
{
        int left = 0;
        int right = (int)g_challengedb_size - 1;
        while (left <= right) {
                int mid = left + ((right - left) / 2);
                int cmp = cg_strcmp(g_challengedb[mid].id_string, id);
                if (cmp == 0) {
                        return &g_challengedb[mid]; /* Found it! */
                }
                if (cmp < 0) {
                        left = mid + 1;
                } else {
                        right = mid - 1;
                }
        }
        return 0;
}
