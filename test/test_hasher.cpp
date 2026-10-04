#include "doctest.h"
extern "C" {
#include "hasher.h"
}
#include "test_utils.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <fcntl>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#endif

static size_t test_digest_size(enum hashmonke_algo algo)
{
    return algo == HASHMONKE_ALGO_MD5 ? 16 : algo == HASHMONKE_ALGO_SHA1 ? 20 : 4;
}

static enum hashmonke_hash_code hash_test_file(
    struct hashmonke_hasher *hasher, const char *path, const uint8_t *expected,
    enum hashmonke_algo algo, bool text_mode = false,
    struct hashmonke_hash_ctrl *out_stats = nullptr)
{
    size_t digest_size = test_digest_size(algo);
    std::vector<uint8_t> buf(sizeof(struct hashmonke_hash) + digest_size);
    struct hashmonke_hash *h = reinterpret_cast<struct hashmonke_hash *>(buf.data());
    h->algo = algo;
    if (expected)
    {
        memcpy(h->value, expected, digest_size);
    }

    struct hashmonke_hash_ctrl stats;
    memset(&stats, 0, sizeof(stats));

    enum hashmonke_hash_code code = hashmonke_hasher_hash(
        hasher, path, text_mode, h, &stats);

    if (out_stats)
    {
        out_stats->bytes_hashed = stats.bytes_hashed;
        out_stats->bytes_total = stats.bytes_total;
        out_stats->ms_elapsed = stats.ms_elapsed;
        out_stats->file_path = stats.file_path;
    }
    return code;
}

static void create_test_file(const std::string &path, const std::string &data)
{
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(data.data(), data.size());
}

static void create_large_test_file(const std::string &path, size_t size, char byte_val)
{
    std::ofstream ofs(path, std::ios::binary);
    std::vector<char> chunk(64 * 1024, byte_val);
    size_t remaining = size;
    while (remaining > 0)
    {
        size_t to_write = std::min(remaining, chunk.size());
        ofs.write(chunk.data(), to_write);
        remaining -= to_write;
    }
}

TEST_CASE("Hasher: Exact Match and Mismatch for MD5, SHA1, SFV")
{
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    std::string test_file = "test_small.tmp";
    create_test_file(test_file, "abc");

    // MD5 exact match
    std::vector<uint8_t> md5_expected = from_hex("900150983cd24fb0d6963f7d28e17f72");
    struct hashmonke_hash_ctrl stats;
    enum hashmonke_hash_code code =
        hash_test_file(hasher, test_file.c_str(), md5_expected.data(), HASHMONKE_ALGO_MD5, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, 3u);

    // MD5 mismatch
    std::vector<uint8_t> md5_wrong = from_hex("00000000000000000000000000000000");
    code = hash_test_file(hasher, test_file.c_str(), md5_wrong.data(), HASHMONKE_ALGO_MD5, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MISMATCH);

    // SHA1 exact match
    std::vector<uint8_t> sha1_expected = from_hex("a9993e364706816aba3e25717850c26c9cd0d89d");
    code = hash_test_file(hasher, test_file.c_str(), sha1_expected.data(), HASHMONKE_ALGO_SHA1, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, 3u);

    // SFV exact match
    std::vector<uint8_t> sfv_expected = from_hex("352441c2");
    code = hash_test_file(hasher, test_file.c_str(), sfv_expected.data(), HASHMONKE_ALGO_SFV, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, 3u);

    // SFV mismatch
    std::vector<uint8_t> sfv_wrong = from_hex("12345678");
    code = hash_test_file(hasher, test_file.c_str(), sfv_wrong.data(), HASHMONKE_ALGO_SFV, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MISMATCH);

    hashmonke_hasher_free(hasher);
    std::remove(test_file.c_str());
}

TEST_CASE("Hasher: Missing file returns IO error")
{
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    create_test_file("test_before_missing.tmp", "abc");
    std::vector<uint8_t> expected = from_hex("900150983cd24fb0d6963f7d28e17f72");
    struct hashmonke_hash_ctrl stats;
    CHECK_EQ(hash_test_file(hasher, "test_before_missing.tmp", expected.data(),
                            HASHMONKE_ALGO_MD5, false, &stats), HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, 3u);
    CHECK_EQ(hash_test_file(hasher, "test_before_missing.tmp", expected.data(),
                            static_cast<enum hashmonke_algo>(99), false, &stats), HASHMONKE_HASH_INTERNAL_ERR);
    CHECK_EQ(stats.bytes_hashed, 0u);

    std::vector<uint8_t> dummy = from_hex("900150983cd24fb0d6963f7d28e17f72");
    enum hashmonke_hash_code code = hash_test_file(
        hasher, "non_existent_file_definitely_not_here.bin", dummy.data(), HASHMONKE_ALGO_MD5, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_IO_ERR);
    CHECK_EQ(stats.bytes_hashed, 0u);

    hashmonke_hasher_free(hasher);
    std::remove("test_before_missing.tmp");
}

TEST_CASE("Hasher: Empty file (0 bytes)")
{
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    std::string empty_file = "test_empty.tmp";
    create_test_file(empty_file, "");

    struct hashmonke_hash_ctrl stats;
    std::vector<uint8_t> md5_empty = from_hex("d41d8cd98f00b204e9800998ecf8427e");
    enum hashmonke_hash_code code =
        hash_test_file(hasher, empty_file.c_str(), md5_empty.data(), HASHMONKE_ALGO_MD5, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, 0u);

    std::vector<uint8_t> sfv_empty = from_hex("00000000");
    code = hash_test_file(hasher, empty_file.c_str(), sfv_empty.data(), HASHMONKE_ALGO_SFV, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, 0u);

    hashmonke_hasher_free(hasher);
    std::remove(empty_file.c_str());
}

TEST_CASE("Hasher: File larger than buffer size through pipeline")
{
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    // Create 9 MiB file filled with character 'A' to exercise repeated buffer reuse.
    size_t file_size = 9 * 1024 * 1024;
    std::string large_file = "test_large_pipeline.tmp";
    create_large_test_file(large_file, file_size, 'A');

    // Compute expected SFV and MD5 reference using algo streaming update
    struct hashmonke_md *sfv_md = hashmonke_md_crc32();
    struct hashmonke_md *md5_md = hashmonke_md_md5();
    std::vector<char> ref_chunk(64 * 1024, 'A');
    size_t rem = file_size;
    while (rem > 0)
    {
        size_t n = std::min(rem, ref_chunk.size());
        hashmonke_md_update_func(sfv_md, ref_chunk.data(), n);
        hashmonke_md_update_func(md5_md, ref_chunk.data(), n);
        rem -= n;
    }
    const uint8_t *sfv_ref = hashmonke_md_final_func(sfv_md);
    const uint8_t *md5_ref = hashmonke_md_final_func(md5_md);

    // Verify through pipeline
    struct hashmonke_hash_ctrl stats;
    enum hashmonke_hash_code code =
        hash_test_file(hasher, large_file.c_str(), sfv_ref, HASHMONKE_ALGO_SFV, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, file_size);

    code = hash_test_file(hasher, large_file.c_str(), md5_ref, HASHMONKE_ALGO_MD5, false, &stats);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, file_size);

    free((void *)sfv_ref);
    free((void *)md5_ref);
    hashmonke_hasher_free(hasher);
    std::remove(large_file.c_str());
}

TEST_CASE("Hasher: Binary data at pipeline buffer boundaries")
{
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    const size_t buffer_size = 1 * 1024 * 1024;
    const std::string path = "test_binary_boundaries.tmp";
    std::string data(2 * buffer_size + 1, '\0');
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<char>((i * 31 + i / 251) & 0xff);

    for (size_t size : {buffer_size - 1, buffer_size, buffer_size + 1,
                        2 * buffer_size, 2 * buffer_size + 1})
    {
        CAPTURE(size);
        const std::string content = data.substr(0, size);
        create_test_file(path, content);
        auto expected = from_hex(hash_string(hashmonke_md_sha1(), content));
        struct hashmonke_hash_ctrl stats;
        CHECK_EQ(hash_test_file(hasher, path.c_str(), expected.data(), HASHMONKE_ALGO_SHA1,
                                false, &stats), HASHMONKE_HASH_MATCHES);
        CHECK_EQ(stats.bytes_hashed, size);
        CHECK_EQ(stats.bytes_total, size);
    }

    hashmonke_hasher_free(hasher);
    std::remove(path.c_str());
}

TEST_CASE("Hasher: Text mode translation vs binary mode")
{
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    const std::string path = "test_text_mode.tmp";
    create_test_file(path, "line1\r\nline2\r\n");

#ifdef _WIN32
    std::string expected_content = "line1\nline2\n";
#else
    std::string expected_content = "line1\r\nline2\r\n";
#endif
    auto expected_text = from_hex(hash_string(hashmonke_md_md5(), expected_content));
    struct hashmonke_hash_ctrl stats;
    CHECK_EQ(hash_test_file(hasher, path.c_str(), expected_text.data(), HASHMONKE_ALGO_MD5, true, &stats),
             HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, expected_content.size());

    std::string raw_content = "line1\r\nline2\r\n";
    auto expected_bin = from_hex(hash_string(hashmonke_md_md5(), raw_content));
    CHECK_EQ(hash_test_file(hasher, path.c_str(), expected_bin.data(), HASHMONKE_ALGO_MD5, false, &stats),
             HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, raw_content.size());

    hashmonke_hasher_free(hasher);
    std::remove(path.c_str());
}

#ifndef _WIN32
TEST_CASE("Hasher: Short reads from a sequential stream")
{
    char directory[] = "/tmp/hashmonke-short-XXXXXX";
    REQUIRE(mkdtemp(directory) != nullptr);
    const std::string path = std::string(directory) + "/input.fifo";
    REQUIRE(mkfifo(path.c_str(), 0600) == 0);
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);
    const std::string data(1 * 1024 * 1024 + 123, 'S');
    auto expected = from_hex(hash_string(hashmonke_md_sha1(), data));
    bool write_ok = true;
    std::thread writer([&] {
        int fd = open(path.c_str(), O_WRONLY);
        if (fd < 0)
        {
            write_ok = false;
            return;
        }
        size_t offset = 0;
        while (offset < data.size())
        {
            ssize_t bytes = write(fd, data.data() + offset,
                                  std::min(size_t(1021), data.size() - offset));
            if (bytes > 0)
                offset += static_cast<size_t>(bytes);
            else if (bytes == 0 || errno != EINTR)
            {
                write_ok = false;
                break;
            }
        }
        close(fd);
    });

    struct hashmonke_hash_ctrl stats;
    auto code = hash_test_file(hasher, path.c_str(), expected.data(), HASHMONKE_ALGO_SHA1, false, &stats);
    writer.join();
    CHECK(write_ok);
    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK_EQ(stats.bytes_hashed, data.size());
    hashmonke_hasher_free(hasher);
    std::remove(path.c_str());
    rmdir(directory);
}
#endif
