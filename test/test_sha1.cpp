#include "doctest.h"
#include "test_utils.hpp"

static std::string sha1_hash_string(const std::string& input) {
    return hash_string(hashmonke_md_sha1(), input);
}

TEST_CASE("SHA1 Standard Test Vectors") {
    CHECK_EQ(sha1_hash_string(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    CHECK_EQ(sha1_hash_string("a"), "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8");
    CHECK_EQ(sha1_hash_string("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK_EQ(sha1_hash_string("message digest"), "c12252ceda8be8994d5fa0290a47231c1d16aae3");
    CHECK_EQ(sha1_hash_string("abcdefghijklmnopqrstuvwxyz"), "32d10c7b8cf96570ca04ce37f2a19d84240d3a89");
    CHECK_EQ(sha1_hash_string("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"), "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST_CASE("SHA1 Streaming Updates") {
    struct hashmonke_md *md = hashmonke_md_sha1();
    std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    for (char c : input) {
        hashmonke_md_update_func(md, &c, 1);
    }
    const uint8_t *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, 20);
    free((void*)digest);
    
    CHECK_EQ(hex, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST_CASE("SHA1 Large Payload (1 million 'a's)") {
    struct hashmonke_md *md = hashmonke_md_sha1();
    std::vector<char> chunk(1000, 'a');
    for (int i = 0; i < 1000; ++i) {
        hashmonke_md_update_func(md, chunk.data(), chunk.size());
    }
    const uint8_t *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, 20);
    free((void*)digest);
    
    CHECK_EQ(hex, "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}

TEST_CASE("SHA1 Interface properties") {
    struct hashmonke_md *md = hashmonke_md_sha1();
    CHECK_EQ(hashmonke_md_digest_size(md), 20u);
    
    const uint8_t *digest = hashmonke_md_final_func(md);
    free((void*)digest);
}
