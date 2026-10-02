#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** based off of openssl's EVP_MD */
struct hashmonke_md;

/** updates the hash context */
void hashmonke_md_update_func(struct hashmonke_md *md, const char *data, size_t len);
/** returns the size of the digest */
size_t hashmonke_md_digest_size(struct hashmonke_md *md);
/** finalizes the hash and returns the digest in a newly allocated buffer */
const char *hashmonke_md_final_func(struct hashmonke_md *md);

struct hashmonke_md *hashmonke_md_md5(void);
struct hashmonke_md *hashmonke_md_sha1(void);
struct hashmonke_md *hashmonke_md_crc32(void);

#ifdef __cplusplus
}
#endif
