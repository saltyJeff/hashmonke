#include "doctest.h"
extern "C" {
#include "file.h"
#include "file_internal.h"
}
#include "test_utils.hpp"
#include <cstdio>
#include <fstream>
#include <string>

static void write_manifest(const std::string &path, const std::string &contents)
{
    std::ofstream out(path, std::ios::binary);
    out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

TEST_CASE("Manifest entries: SFV paths and CRC32 digests")
{
    const std::string path = "test_entries.sfv";
    write_manifest(path, "; comment\nalpha.bin 352441c2\nfolder\\ item.bin cbf43926\n");
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(file != nullptr);
    CHECK_EQ(file->format, HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE_EQ(file->num_entries, 2u);
    CHECK(std::string(file->entries[0].file_path).find("alpha.bin") != std::string::npos);
    CHECK_EQ(file->entries[0].algo, HASHMONKE_ALGO_SFV);
    CHECK_EQ(to_hex(file->entries[0].hash, 4), "352441c2");
    CHECK_FALSE(file->entries[0].text_mode);
    CHECK(std::string(file->entries[1].file_path).find("folder item.bin") != std::string::npos);
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: GNU MD5, SHA1 and text mode marker")
{
    const std::string path = "test_entries.md5";
    write_manifest(path,
        "900150983cd24fb0d6963f7d28e17f72  alpha.txt\n"
        "\\c3fcd3d76192e4007dfb496cca67e13b *binary.bin\n"
        "da39a3ee5e6b4b0d3255bfef95601890afd80709  empty.sha1\n");
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(file != nullptr);
    REQUIRE_EQ(file->num_entries, 3u);
    CHECK_EQ(file->entries[0].algo, HASHMONKE_ALGO_MD5);
    CHECK(file->entries[0].text_mode);
    CHECK_EQ(to_hex(file->entries[0].hash, 16), "900150983cd24fb0d6963f7d28e17f72");
    CHECK_EQ(file->entries[1].algo, HASHMONKE_ALGO_MD5);
    CHECK_FALSE(file->entries[1].text_mode);
    CHECK_EQ(file->entries[2].algo, HASHMONKE_ALGO_SHA1);
    CHECK_EQ(to_hex(file->entries[2].hash, 20), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: BSD formats and autodetection")
{
    const std::string path = "test_entries.txt";
    write_manifest(path, "\xef\xbb\xbfMD5 (alpha.bin) = 900150983cd24fb0d6963f7d28e17f72\n"
                          "SHA-1 (beta.bin) = da39a3ee5e6b4b0d3255bfef95601890afd80709\n");
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_AUTODETECT);
    REQUIRE(file != nullptr);
    CHECK_EQ(file->format, HASHMONKE_FILE_FORMAT_BSD);
    REQUIRE_EQ(file->num_entries, 2u);
    CHECK_EQ(file->entries[0].algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(file->entries[1].algo, HASHMONKE_ALGO_SHA1);
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: malformed records are retained for callers to skip")
{
    const std::string path = "test_entries_bad.md5";
    write_manifest(path,
        "900150983cd24fb0d6963f7d28e17f72  first.txt\n"
        "this is malformed\n"
        "d41d8cd98f00b204e9800998ecf8427e *second.bin\n"
        "shortbadhex  broken.txt\n"
        "c3fcd3d76192e4007dfb496cca67e13b  third.txt\n");
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(file != nullptr);
    REQUIRE_EQ(file->num_entries, 5u);
    CHECK_EQ(file->entries[0].code, HASHMONKE_ENTRY_OK);
    CHECK_EQ(file->entries[1].code, HASHMONKE_ENTRY_ERR);
    CHECK(file->entries[1].file_path == nullptr);
    CHECK(file->entries[1].hash == nullptr);
    CHECK_EQ(file->entries[2].code, HASHMONKE_ENTRY_OK);
    CHECK_EQ(file->entries[3].code, HASHMONKE_ENTRY_ERR);
    CHECK_EQ(file->entries[4].code, HASHMONKE_ENTRY_OK);
    CHECK(std::string(file->entries[0].file_path).find("first.txt") != std::string::npos);
    CHECK(std::string(file->entries[2].file_path).find("second.bin") != std::string::npos);
    CHECK(std::string(file->entries[4].file_path).find("third.txt") != std::string::npos);
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: BOM is skipped during autodetection and parsing")
{
    const std::string path = "test_entries_bom.txt";
    write_manifest(path, "\xef\xbb\xbfMD5 (data.bin) = 900150983cd24fb0d6963f7d28e17f72\n");
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_AUTODETECT);
    REQUIRE(file != nullptr);
    CHECK_EQ(file->format, HASHMONKE_FILE_FORMAT_BSD);
    REQUIRE_EQ(file->num_entries, 1u);
    CHECK_EQ(file->entries[0].code, HASHMONKE_ENTRY_OK);
    CHECK_EQ(file->entries[0].algo, HASHMONKE_ALGO_MD5);
    CHECK_EQ(to_hex(file->entries[0].hash, 16), "900150983cd24fb0d6963f7d28e17f72");
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: long lines and paths are read intact")
{
    const std::string path = "test_entries_long.sfv";
    const std::string long_name(5000, 'x');
    write_manifest(path, ";" + std::string(6000, 'c') + "\n" + long_name + " 352441c2\nnext.bin cbf43926\n");
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(file != nullptr);
    REQUIRE_EQ(file->num_entries, 2u);
    CHECK_EQ(file->entries[0].code, HASHMONKE_ENTRY_OK);
    CHECK(std::string(file->entries[0].file_path).find(long_name) != std::string::npos);
    CHECK_EQ(to_hex(file->entries[0].hash, 4), "352441c2");
    CHECK(std::string(file->entries[1].file_path).find("next.bin") != std::string::npos);
    CHECK_EQ(to_hex(file->entries[1].hash, 4), "cbf43926");
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: folder override resolves relative file paths")
{
    const std::string path = "test_entries_override.md5";
    write_manifest(path, "900150983cd24fb0d6963f7d28e17f72  sub/file.txt\n");
    struct hashmonke_file *file = hashmonke_file_init("algo", path.c_str(), HASHMONKE_FILE_FORMAT_GNU_MD5);
    REQUIRE(file != nullptr);
    REQUIRE_EQ(file->num_entries, 1u);
    CHECK(std::string(file->entries[0].file_path).find("algo") != std::string::npos);
    const std::string resolved_path = file->entries[0].file_path;
    const bool has_resolved_suffix = resolved_path.find("sub/file.txt") != std::string::npos ||
                                     resolved_path.find("sub\\file.txt") != std::string::npos;
    CHECK(has_resolved_suffix);
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

TEST_CASE("Manifest entries: dynamic array growth handles variable counts")
{
    const std::string path = "test_entries_growth.sfv";
    std::string content;
    for (int i = 0; i < 65; ++i)
    {
        content += "file" + std::to_string(i) + ".bin 352441c2\n";
    }
    write_manifest(path, content);
    struct hashmonke_file *file = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(file != nullptr);
    CHECK_EQ(file->num_entries, 65u);
    for (size_t i = 0; i < 65; ++i)
    {
        CHECK_EQ(file->entries[i].code, HASHMONKE_ENTRY_OK);
        CHECK_EQ(file->entries[i].algo, HASHMONKE_ALGO_SFV);
    }
    hashmonke_file_free(file);
    std::remove(path.c_str());
}

