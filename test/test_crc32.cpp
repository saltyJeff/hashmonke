#include "doctest.h"
#include "test_utils.hpp"

static std::string crc32_hash_string(const std::string &input)
{
    return hash_string(hashmonke_md_crc32(), input);
}

TEST_CASE("CRC32 Standard Test Vectors")
{
    CHECK_EQ(crc32_hash_string(""), "00000000");
    CHECK_EQ(crc32_hash_string("a"), "e8b7be43");
    CHECK_EQ(crc32_hash_string("abc"), "352441c2");
    CHECK_EQ(crc32_hash_string("message digest"), "20159d7f");
    CHECK_EQ(crc32_hash_string("123456789"), "cbf43926");
    CHECK_EQ(crc32_hash_string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"), "1fc2e6d2");
}

TEST_CASE("CRC32 Streaming Updates")
{
    struct hashmonke_md *md = hashmonke_md_crc32();
    std::string input = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (char c : input)
    {
        hashmonke_md_update_func(md, &c, 1);
    }
    const char *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, 4);
    free((void *)digest);

    CHECK_EQ(hex, "1fc2e6d2");
}

TEST_CASE("CRC32 Large Payload (1 million 'a's)")
{
    struct hashmonke_md *md = hashmonke_md_crc32();
    std::vector<char> chunk(1000, 'a');
    for (int i = 0; i < 1000; ++i)
    {
        hashmonke_md_update_func(md, chunk.data(), chunk.size());
    }
    const char *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, 4);
    free((void *)digest);

    CHECK_EQ(hex, "dc25bfbc");
}

TEST_CASE("CRC32 Interface properties")
{
    struct hashmonke_md *md = hashmonke_md_crc32();
    CHECK_EQ(hashmonke_md_digest_size(md), 4u);

    const char *digest = hashmonke_md_final_func(md);
    free((void *)digest);
}
