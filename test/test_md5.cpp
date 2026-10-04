#include "doctest.h"
#include "test_utils.hpp"

static std::string md5_hash_string(const std::string &input)
{
    return hash_string(hashmonke_md_md5(), input);
}

TEST_CASE("MD5 RFC 1321 Test Vectors")
{
    CHECK_EQ(md5_hash_string(""), "d41d8cd98f00b204e9800998ecf8427e");
    CHECK_EQ(md5_hash_string("a"), "0cc175b9c0f1b6a831c399e269772661");
    CHECK_EQ(md5_hash_string("abc"), "900150983cd24fb0d6963f7d28e17f72");
    CHECK_EQ(md5_hash_string("message digest"), "f96b697d7cb7938d525a2f31aaf161d0");
    CHECK_EQ(md5_hash_string("abcdefghijklmnopqrstuvwxyz"), "c3fcd3d76192e4007dfb496cca67e13b");
    CHECK(md5_hash_string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") ==
          "d174ab98d277d9f5a5611c2c9f419d9f");
    CHECK(md5_hash_string("12345678901234567890123456789012345678901234567890123456789012345678901234567890") ==
          "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_CASE("MD5 Streaming Updates")
{
    struct hashmonke_md *md = hashmonke_md_md5();
    std::string input = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (char c : input)
    {
        hashmonke_md_update_func(md, &c, 1);
    }
    const uint8_t *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, 16);
    free((void *)digest);

    CHECK_EQ(hex, "d174ab98d277d9f5a5611c2c9f419d9f");
}

TEST_CASE("MD5 Large Payload (1 million 'a's)")
{
    struct hashmonke_md *md = hashmonke_md_md5();
    std::vector<char> chunk(1000, 'a');
    for (int i = 0; i < 1000; ++i)
    {
        hashmonke_md_update_func(md, chunk.data(), chunk.size());
    }
    const uint8_t *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, 16);
    free((void *)digest);

    CHECK_EQ(hex, "7707d6ae4e027c70eea2a935c2296f21");
}

TEST_CASE("MD5 Interface properties")
{
    struct hashmonke_md *md = hashmonke_md_md5();
    CHECK_EQ(hashmonke_md_digest_size(md), 16u);

    // final automatically frees context
    const uint8_t *digest = hashmonke_md_final_func(md);
    free((void *)digest);
}
