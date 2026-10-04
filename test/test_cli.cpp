extern "C" {
#include "cli.h"
}
#include "doctest.h"
#include <vector>

TEST_CASE("CLI: Help flag")
{
    const char *argv1[] = {"hashmonke", "--help"};
    struct hashmonke_cli cli1 = hashmonke_parse_cli(2, argv1);
    CHECK_EQ(cli1.task, HASHMONKE_CLI_HELP);

    const char *argv2[] = {"hashmonke", "-h"};
    struct hashmonke_cli cli2 = hashmonke_parse_cli(2, argv2);
    CHECK_EQ(cli2.task, HASHMONKE_CLI_HELP);
}

TEST_CASE("CLI: Valid arguments and options")
{
    // Basic file
    const char *argv1[] = {"hashmonke", "archive.sfv"};
    struct hashmonke_cli cli1 = hashmonke_parse_cli(2, argv1);
    CHECK_EQ(cli1.task, HASHMONKE_CLI_VERIFY);
    CHECK_EQ(std::string(cli1.hash_file), "archive.sfv");
    CHECK_EQ(cli1.format, HASHMONKE_FILE_FORMAT_AUTODETECT);
    CHECK(cli1.folder == nullptr);

    // With folder and format
    const char *argv2[] = {"hashmonke", "hashes.md5", "--folder", "/data/files", "--format", "gnu"};
    struct hashmonke_cli cli2 = hashmonke_parse_cli(6, argv2);
    CHECK_EQ(cli2.task, HASHMONKE_CLI_VERIFY);
    CHECK_EQ(std::string(cli2.hash_file), "hashes.md5");
    CHECK_EQ(std::string(cli2.folder), "/data/files");
    CHECK_EQ(cli2.format, HASHMONKE_FILE_FORMAT_GNU_MD5);

    // Formats with equals syntax
    const char *argv3[] = {"hashmonke", "--format=bsd", "--folder=./sub", "manifest.txt"};
    struct hashmonke_cli cli3 = hashmonke_parse_cli(4, argv3);
    CHECK_EQ(cli3.task, HASHMONKE_CLI_VERIFY);
    CHECK_EQ(std::string(cli3.hash_file), "manifest.txt");
    CHECK_EQ(std::string(cli3.folder), "./sub");
    CHECK_EQ(cli3.format, HASHMONKE_FILE_FORMAT_BSD);

    // Format sfv
    const char *argv4[] = {"hashmonke", "test.sfv", "--format", "sfv"};
    struct hashmonke_cli cli4 = hashmonke_parse_cli(4, argv4);
    CHECK_EQ(cli4.task, HASHMONKE_CLI_VERIFY);
    CHECK_EQ(cli4.format, HASHMONKE_FILE_FORMAT_SFV);
}

TEST_CASE("CLI: Errors and validation")
{
    // No arguments
    const char *argv1[] = {"hashmonke"};
    struct hashmonke_cli cli1 = hashmonke_parse_cli(1, argv1);
    CHECK_EQ(cli1.task, HASHMONKE_CLI_ERR);

    // Missing folder argument
    const char *argv2[] = {"hashmonke", "file.sfv", "--folder"};
    struct hashmonke_cli cli2 = hashmonke_parse_cli(3, argv2);
    CHECK_EQ(cli2.task, HASHMONKE_CLI_ERR);

    // Missing format argument
    const char *argv3[] = {"hashmonke", "file.sfv", "--format"};
    struct hashmonke_cli cli3 = hashmonke_parse_cli(3, argv3);
    CHECK_EQ(cli3.task, HASHMONKE_CLI_ERR);

    // Invalid format
    const char *argv4[] = {"hashmonke", "file.sfv", "--format", "unknown_format"};
    struct hashmonke_cli cli4 = hashmonke_parse_cli(4, argv4);
    CHECK_EQ(cli4.task, HASHMONKE_CLI_ERR);

    // Multiple checksum files
    const char *argv5[] = {"hashmonke", "file1.sfv", "file2.sfv"};
    struct hashmonke_cli cli5 = hashmonke_parse_cli(3, argv5);
    CHECK_EQ(cli5.task, HASHMONKE_CLI_ERR);

    // Unknown option
    const char *argv6[] = {"hashmonke", "file.sfv", "--invalid-flag"};
    struct hashmonke_cli cli6 = hashmonke_parse_cli(3, argv6);
    CHECK_EQ(cli6.task, HASHMONKE_CLI_ERR);
}
