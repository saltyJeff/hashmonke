#include "doctest.h"
#include "file.h"
#include "runner.h"
#include "test_utils.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

static void create_file(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(content.data(), content.size());
}

struct CallbackRecord
{
    std::string path;
    enum hashmonke_hash_code code;
};

static std::mutex g_cb_mutex;
static std::vector<CallbackRecord> g_cb_records;
static std::atomic<bool> g_blocking_callback_entered{false};
static std::atomic<bool> g_release_blocking_callback{false};

static void blocking_runner_callback(const char *, enum hashmonke_hash_code)
{
    if (!g_blocking_callback_entered.exchange(true))
    {
        while (!g_release_blocking_callback.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

static void test_runner_callback(const char *path, enum hashmonke_hash_code code)
{
    std::lock_guard<std::mutex> lock(g_cb_mutex);
    g_cb_records.push_back({path ? path : "", code});
}

TEST_CASE("Runner: Execution with match, mismatch, missing, and malformed records")
{
    // Create actual test files
    create_file("test_match1.bin", "file1 content");
    create_file("test_match2.bin", "file2 content");
    create_file("test_mismatch.bin", "actual content");
    // test_missing.bin is intentionally not created!

    // Compute expected hashes
    std::string match1_crc = hash_string(hashmonke_md_crc32(), "file1 content");
    std::string match2_crc = hash_string(hashmonke_md_crc32(), "file2 content");

    std::string manifest_content =
        "test_match1.bin " + match1_crc + "\n" +
        "test_match2.bin " + match2_crc + "\n" +
        "test_mismatch.bin 00000000\n" +
        "test_missing.bin 12345678\n" +
        "malformed line without valid crc\n";

    std::string manifest_path = "test_runner_manifest.sfv";
    create_file(manifest_path, manifest_content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, manifest_path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(f != nullptr);

    {
        std::lock_guard<std::mutex> lock(g_cb_mutex);
        g_cb_records.clear();
    }

    struct hashmonke_runner *runner = hashmonke_runner_run(f, test_runner_callback);
    REQUIRE(runner != nullptr);

    hashmonke_runner_wait(runner);
    hashmonke_runner_wait(runner);

    struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
    CHECK(stats.is_finished);
    CHECK_EQ(stats.files_matched, 2u);
    CHECK_EQ(stats.files_failed, 1u);
    CHECK_EQ(stats.files_missing, 1u);
    CHECK_EQ(stats.files_malformed, 1u);
    CHECK_EQ(stats.total_files_processed, 5u);
    CHECK(stats.total_bytes_hashed > 0);

    {
        std::lock_guard<std::mutex> lock(g_cb_mutex);
        CHECK_EQ(g_cb_records.size(), 5u);
    }

    hashmonke_runner_free(runner);
    hashmonke_file_free(f);

    std::remove("test_match1.bin");
    std::remove("test_match2.bin");
    std::remove("test_mismatch.bin");
    std::remove(manifest_path.c_str());
}

TEST_CASE("Runner: Cancellation / Interruption")
{
    // Generate large manifest
    std::string manifest_content;
    for (int i = 0; i < 500; ++i)
    {
        manifest_content += "dummy_nonexistent_" + std::to_string(i) + ".bin 12345678\n";
    }

    std::string manifest_path = "test_runner_cancel.sfv";
    create_file(manifest_path, manifest_content);

    struct hashmonke_file *f = hashmonke_file_init(nullptr, manifest_path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(f != nullptr);

    g_blocking_callback_entered.store(false);
    g_release_blocking_callback.store(false);
    struct hashmonke_runner *runner = hashmonke_runner_run(f, blocking_runner_callback);
    REQUIRE(runner != nullptr);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!g_blocking_callback_entered.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    CHECK(g_blocking_callback_entered.load());
    hashmonke_runner_interrupt(runner);
    g_release_blocking_callback.store(true);

    hashmonke_runner_wait(runner);

    struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
    CHECK(stats.is_finished);
    CHECK_EQ(stats.active_workers, 0u);
    CHECK(stats.total_files_processed > 0u);
    CHECK(stats.total_files_processed < 500u);

    hashmonke_runner_free(runner);
    hashmonke_file_free(f);
    std::remove(manifest_path.c_str());
}

#ifndef _WIN32
TEST_CASE("Runner: Manifest read errors fail the run")
{
    const std::string directory = "test_manifest_read_error.tmpdir";
    std::filesystem::remove_all(directory);
    REQUIRE(std::filesystem::create_directory(directory));

    struct hashmonke_file *file = hashmonke_file_init(nullptr, directory.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(file != nullptr);
    struct hashmonke_runner *runner = hashmonke_runner_run(file, nullptr);
    REQUIRE(runner != nullptr);
    hashmonke_runner_wait(runner);

    const struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
    CHECK(stats.is_finished);
    CHECK(stats.has_error);
    CHECK_EQ(stats.total_files_processed, 0u);

    hashmonke_runner_free(runner);
    hashmonke_file_free(file);
    std::filesystem::remove_all(directory);
}
#endif

#ifndef _WIN32
TEST_CASE("Runner: Manifest read errors fail the run")
{
    const std::string directory = "test_manifest_read_error.tmpdir";
    std::filesystem::remove_all(directory);
    REQUIRE(std::filesystem::create_directory(directory));

    struct hashmonke_file *file = hashmonke_file_init(nullptr, directory.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(file != nullptr);
    struct hashmonke_runner *runner = hashmonke_runner_run(file, nullptr);
    REQUIRE(runner != nullptr);
    hashmonke_runner_wait(runner);

    const struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
    CHECK(stats.is_finished);
    CHECK(stats.has_error);
    CHECK_EQ(stats.total_files_processed, 0u);

    hashmonke_runner_free(runner);
    hashmonke_file_free(file);
    std::filesystem::remove_all(directory);
}
#endif
