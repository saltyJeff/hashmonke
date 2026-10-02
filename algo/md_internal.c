#include "md_internal.h"
#include <stdlib.h>

void hashmonke_md_update_func(struct hashmonke_md *md, const char *data, size_t len)
{
    if (md && md->update)
    {
        md->update(md, data, len);
    }
}

size_t hashmonke_md_digest_size(struct hashmonke_md *md)
{
    if (md)
    {
        return md->digest_size;
    }
    return 0;
}

const char *hashmonke_md_final_func(struct hashmonke_md *md)
{
    if (md && md->final)
    {
        return md->final(md);
    }
    return NULL;
}
