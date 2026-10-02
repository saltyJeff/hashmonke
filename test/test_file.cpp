#include "doctest.h"
#include "file.h"
#include "file_internal.h"
#include "hash.h"
#include "test_utils.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

static void release_test_tuple(struct hashmonke_hash_tuple *tuple)
{
    if (tuple->file_fd >= 0)
    {
#ifdef _WIN32
        _close(tuple->file_fd);
#else
        close(tuple->file_fd);
#endif
    }
    free(tuple->hash);
    free(tuple->file_abs_path);
    free(tuple->raw_line);
}

static void write_temp_file(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(content.data(), content.size());
}

TEST_CASE("Manifest: SFV parsing and comments")
{
    std::string sfv_content =
        "; Sample SFV file\n"
        "# Another comment\n"
        "\n"
        "test1.txt 352441c2\n"
        "file with spaces.bin 1fc2e6d2\n"
        "escaped\\ space.dat cbf43926\n";

    std::string tmp_manifest = "test_sample.sfv";
    write_temp_file(tmp_manifest, sfv_content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(f != nullptr);
    CHECK_EQ(hashmonke_file_get_format(f), HASHMONKE_FILE_FORMAT_SFV);

    struct hashmonke_hash_tuple tup;

    // 1st tuple
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_CRC32);
    CHECK_EQ(tup.line_num, 4u);
    CHECK(tup.file_abs_path != nullptr);
    CHECK(std::string(tup.file_abs_path).find("test1.txt") != std::string::npos);
    std::string hex1 = to_hex(tup.hash, 4);
    CHECK_EQ(hex1, "352441c2");
    release_test_tuple(&tup);

    // 2nd tuple
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_CRC32);
    CHECK_EQ(tup.line_num, 5u);
    CHECK(tup.file_abs_path != nullptr);
    CHECK(std::string(tup.file_abs_path).find("file with spaces.bin") != std::string::npos);
    std::string hex2 = to_hex(tup.hash, 4);
    CHECK_EQ(hex2, "1fc2e6d2");
    release_test_tuple(&tup);

    // 3rd tuple (escaped space)
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_CRC32);
    CHECK_EQ(tup.line_num, 6u);
    CHECK(tup.file_abs_path != nullptr);
    CHECK(std::string(tup.file_abs_path).find("escaped space.dat") != std::string::npos);
    std::string hex3 = to_hex(tup.hash, 4);
    CHECK_EQ(hex3, "cbf43926");
    release_test_tuple(&tup);

    // EOF
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_EOF);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Manifest: GNU MD5 and SHA1 format")
{
    std::string gnu_content =
        "900150983cd24fb0d6963f7d28e17f72  abc.txt\n"
        "d41d8cd98f00b204e9800998ecf8427e *binary_empty.bin\n"
        "\\c3fcd3d76192e4007dfb496cca67e13b  escaped\\ name.bin\n"
        "da39a3ee5e6b4b0d3255bfef95601890afd80709  empty.sha1\n";

    std::string tmp_manifest = "test_sample.md5";
    write_temp_file(tmp_manifest, gnu_content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(f != nullptr);

    struct hashmonke_hash_tuple tup;

    // Tuple 1: MD5 text mode
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(tup.line_num, 1u);
    CHECK_EQ(to_hex(tup.hash, 16), "900150983cd24fb0d6963f7d28e17f72");
    CHECK(std::string(tup.file_abs_path).find("abc.txt") != std::string::npos);
    release_test_tuple(&tup);

    // Tuple 2: MD5 binary mode (*)
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(tup.line_num, 2u);
    CHECK_EQ(to_hex(tup.hash, 16), "d41d8cd98f00b204e9800998ecf8427e");
    CHECK(std::string(tup.file_abs_path).find("binary_empty.bin") != std::string::npos);
    release_test_tuple(&tup);

    // Tuple 3: Escaped leading backslash
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(tup.line_num, 3u);
    CHECK_EQ(to_hex(tup.hash, 16), "c3fcd3d76192e4007dfb496cca67e13b");
    CHECK(std::string(tup.file_abs_path).find("escaped name.bin") != std::string::npos);
    release_test_tuple(&tup);

    // Tuple 4: SHA1 40-hex
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_SHA1);
    CHECK_EQ(tup.line_num, 4u);
    CHECK_EQ(to_hex(tup.hash, 20), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    CHECK(std::string(tup.file_abs_path).find("empty.sha1") != std::string::npos);
    release_test_tuple(&tup);

    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_EOF);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Manifest tuples expose opened descriptors and GNU mode marker")
{
    const std::string target = "test_gnu_modes.tmp";
    const std::string manifest = "test_gnu_modes.md5";
    const std::string raw = "A\r\nB";
    write_temp_file(target, raw);
#ifdef _WIN32
    const std::string text_mode_bytes = "A\nB";
#else
    const std::string text_mode_bytes = raw;
#endif
    std::string content = hash_string(hashmonke_md_md5(), text_mode_bytes) +
                          "  " + target + "\n" +
                          hash_string(hashmonke_md_md5(), raw) +
                          " *" + target + "\n" +
                          "d41d8cd98f00b204e9800998ecf8427e  absent_gnu_modes.tmp\n";
    write_temp_file(manifest, content);
    struct hashmonke_file *file = hashmonke_file_init(nullptr, manifest.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(file != nullptr);
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);
    struct hashmonke_hash_tuple tuple;
    REQUIRE_EQ(hashmonke_file_next_tuple(file, &tuple), HASHMONKE_IT_OK);
    CHECK(tuple.file_fd >= 0);
    int fd = tuple.file_fd;
    CHECK_EQ(hashmonke_hasher_hash(hasher, fd, tuple.hash,
                 HASHMONKE_ALGO_MD5, nullptr, nullptr), HASHMONKE_HASH_MATCHES);
    tuple.file_fd = -1;
    tuple.hash = nullptr;
    release_test_tuple(&tuple);

    REQUIRE_EQ(hashmonke_file_next_tuple(file, &tuple), HASHMONKE_IT_OK);
    CHECK(tuple.file_fd >= 0);
    fd = tuple.file_fd;
    CHECK_EQ(hashmonke_hasher_hash(hasher, fd, tuple.hash,
                 HASHMONKE_ALGO_MD5, nullptr, nullptr), HASHMONKE_HASH_MATCHES);
    tuple.file_fd = -1;
    tuple.hash = nullptr;
    release_test_tuple(&tuple);

    REQUIRE_EQ(hashmonke_file_next_tuple(file, &tuple), HASHMONKE_IT_OK);
    CHECK_EQ(tuple.file_fd, -1);
    release_test_tuple(&tuple);
    hashmonke_hasher_free(hasher);
    hashmonke_file_free(file);
    std::remove(target.c_str());
    std::remove(manifest.c_str());
}

TEST_CASE("Manifest: BSD format")
{
    std::string bsd_content =
        "MD5 (foo/bar.txt) = 900150983cd24fb0d6963f7d28e17f72\n"
        "SHA1 (baz.bin) = a9993e364706816aba3e25717850c26c9cd0d89d\n"
        "CRC32 (checksum.sfv) = 352441c2\n";

    std::string tmp_manifest = "test_bsd.txt";
    write_temp_file(tmp_manifest, bsd_content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_BSD);
    REQUIRE(f != nullptr);

    struct hashmonke_hash_tuple tup;

    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(to_hex(tup.hash, 16), "900150983cd24fb0d6963f7d28e17f72");
    release_test_tuple(&tup);

    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_SHA1);
    CHECK_EQ(to_hex(tup.hash, 20), "a9993e364706816aba3e25717850c26c9cd0d89d");
    release_test_tuple(&tup);

    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_CRC32);
    CHECK_EQ(to_hex(tup.hash, 4), "352441c2");
    release_test_tuple(&tup);

    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_EOF);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Manifest: UTF-8 BOM and Autodetection")
{
    std::string bom_content = "\xEF\xBB\xBFMD5 (data.bin) = 900150983cd24fb0d6963f7d28e17f72\n";
    std::string tmp_manifest = "test_bom_autodetect.txt";
    write_temp_file(tmp_manifest, bom_content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_AUTODETECT);
    REQUIRE(f != nullptr);
    CHECK_EQ(hashmonke_file_get_format(f), HASHMONKE_FILE_FORMAT_BSD);

    struct hashmonke_hash_tuple tup;
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(tup.line_num, 1u);
    CHECK_EQ(to_hex(tup.hash, 16), "900150983cd24fb0d6963f7d28e17f72");
    release_test_tuple(&tup);

    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_EOF);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Manifest: Path resolution override via folder_path")
{
    std::string content = "900150983cd24fb0d6963f7d28e17f72  sub/file.txt\n";
    std::string tmp_manifest = "test_override.md5";
    write_temp_file(tmp_manifest, content);

    struct hashmonke_file *f = hashmonke_file_init("algo", tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(f != nullptr);

    struct hashmonke_hash_tuple tup;
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    std::string path(tup.file_abs_path);
    CHECK(path.find("algo") != std::string::npos);
    CHECK(path.find("sub") != std::string::npos);
    CHECK(path.find("file.txt") != std::string::npos);
    release_test_tuple(&tup);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Manifest: Malformed line tolerance")
{
    std::string content =
        "900150983cd24fb0d6963f7d28e17f72  valid1.txt\n"
        "this is totally malformed\n"
        "d41d8cd98f00b204e9800998ecf8427e  valid2.txt\n"
        "shortbadhex  bad.txt\n"
        "c3fcd3d76192e4007dfb496cca67e13b  valid3.txt\n";

    std::string tmp_manifest = "test_malformed.md5";
    write_temp_file(tmp_manifest, content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(f != nullptr);

    struct hashmonke_hash_tuple tup;

    // Line 1: valid
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.line_num, 1u);
    CHECK(std::string(tup.file_abs_path).find("valid1.txt") != std::string::npos);
    release_test_tuple(&tup);

    // Line 2: malformed
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_MALFORMED);
    CHECK_EQ(tup.line_num, 2u);
    CHECK(tup.raw_line != nullptr);
    CHECK(std::string(tup.raw_line).find("this is totally malformed") != std::string::npos);
    CHECK(tup.hash == nullptr);
    CHECK(tup.file_abs_path == nullptr);
    release_test_tuple(&tup);

    // Line 3: valid
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.line_num, 3u);
    CHECK(std::string(tup.file_abs_path).find("valid2.txt") != std::string::npos);
    release_test_tuple(&tup);

    // Line 4: malformed
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_MALFORMED);
    CHECK_EQ(tup.line_num, 4u);
    CHECK(tup.raw_line != nullptr);
    CHECK(std::string(tup.raw_line).find("shortbadhex") != std::string::npos);
    release_test_tuple(&tup);

    // Line 5: valid
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_OK);
    CHECK_EQ(tup.line_num, 5u);
    CHECK(std::string(tup.file_abs_path).find("valid3.txt") != std::string::npos);
    release_test_tuple(&tup);

    // Line 6: EOF
    REQUIRE_EQ(hashmonke_file_next_tuple(f, &tup), HASHMONKE_IT_EOF);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Manifest: Thread-safe concurrent iterator access")
{
    std::string content;
    for (int i = 0; i < 100; ++i)
    {
        content += "file_" + std::to_string(i) + ".bin 1fc2e6d2\n";
    }

    std::string tmp_manifest = "test_concurrent.sfv";
    write_temp_file(tmp_manifest, content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, tmp_manifest.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(f != nullptr);

    std::atomic<int> ok_count{0};
    std::mutex seen_mutex;
    std::unordered_set<size_t> seen_lines;
    std::vector<std::thread> workers;

    for (int w = 0; w < 4; ++w)
    {
        workers.emplace_back([f, &ok_count, &seen_mutex, &seen_lines]() {
            struct hashmonke_hash_tuple tup;
            while (true)
            {
                enum hashmonke_it_code code = hashmonke_file_next_tuple(f, &tup);
                if (code == HASHMONKE_IT_OK)
                {
                    ok_count++;
                    {
                        std::lock_guard<std::mutex> guard(seen_mutex);
                        seen_lines.insert(tup.line_num);
                    }
                    release_test_tuple(&tup);
                }
                else
                {
                    break;
                }
            }
        });
    }

    for (auto &w : workers)
    {
        w.join();
    }

    CHECK_EQ(ok_count.load(), 100);
    CHECK_EQ(seen_lines.size(), 100u);
    for (size_t line = 1; line <= 100; ++line)
        CHECK(seen_lines.count(line) == 1);

    hashmonke_file_free(f);
    std::remove(tmp_manifest.c_str());
}

TEST_CASE("Path: hashmonke_resolve_entry_path resolution")
{
    // Relative with no trailing slash
    char *p1 = hashmonke_resolve_entry_path("folder", "sub/file.txt");
    REQUIRE(p1 != nullptr);
    CHECK(strstr(p1, "file.txt") != nullptr);
    CHECK(std::filesystem::path(p1).is_absolute());
    free(p1);

    // Relative with trailing slash
    char *p2 = hashmonke_resolve_entry_path("folder/", "sub/file.txt");
    REQUIRE(p2 != nullptr);
    CHECK(strstr(p2, "file.txt") != nullptr);
    CHECK(std::filesystem::path(p2).is_absolute());
    free(p2);

    // Null or empty base
    char *p3 = hashmonke_resolve_entry_path(nullptr, "test_file.cpp");
    REQUIRE(p3 != nullptr);
    CHECK(std::filesystem::path(p3).is_absolute());
    free(p3);
}

TEST_CASE("Manifest: Lines longer than the old fixed buffer are read intact")
{
    std::string long_name(5000, 'x');
    std::string manifest_path = "test_long_line.sfv";
    write_temp_file(manifest_path, "#" + std::string(6000, 'c') + "\n" + long_name + " 352441c2\n" +
                                       "next.bin cbf43926\n");

    struct hashmonke_file *file = hashmonke_file_init(nullptr, manifest_path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(file != nullptr);
    struct hashmonke_hash_tuple tuple;
    REQUIRE_EQ(hashmonke_file_next_tuple(file, &tuple), HASHMONKE_IT_OK);
    CHECK_EQ(tuple.line_num, 2u);
    CHECK(std::string(tuple.file_abs_path).find(long_name) != std::string::npos);
    release_test_tuple(&tuple);
    REQUIRE_EQ(hashmonke_file_next_tuple(file, &tuple), HASHMONKE_IT_OK);
    CHECK_EQ(tuple.line_num, 3u);
    CHECK(std::string(tuple.file_abs_path).find("next.bin") != std::string::npos);
    release_test_tuple(&tuple);
    CHECK_EQ(hashmonke_file_next_tuple(file, &tuple), HASHMONKE_IT_EOF);
    hashmonke_file_free(file);
    std::remove(manifest_path.c_str());
}

