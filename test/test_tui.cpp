#include "doctest.h"

extern "C" {
#include "file.h"
#include "hasher.h"
#include "runner.h"
#include "tui.h"
#include "tui_render.h"
}

#include "test_utils.hpp"
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

static void create_test_file(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(content.data(), content.size());
}

TEST_CASE("TUI Plan: hashmonke_file_list scans manifest without opening files")
{
    std::string manifest_path = "test_file_list_manifest.sfv";
    std::string content =
        "; Comment line 1\n"
        "\n"
        "photos/01.jpg 12345678\n"
        "; Comment line 4\n"
        "videos/clip.mp4 87654321\n";
    create_test_file(manifest_path, content);

    // Note: neither photos/01.jpg nor videos/clip.mp4 exist on disk.
    // hashmonke_file_list must scan successfully and collect entries without failing on missing files.
    struct hashmonke_file_list *list = hashmonke_file_list(nullptr, manifest_path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(list != nullptr);
    REQUIRE_EQ(list->num_entries, 2u);

    CHECK_EQ(list->entries[0].line_number, 3u);
    CHECK(list->entries[0].display_path != nullptr);
    CHECK_EQ(std::string(list->entries[0].display_path), "photos/01.jpg");
    CHECK(list->entries[0].file_path != nullptr);
    CHECK_EQ(list->entries[0].code, HASHMONKE_ENTRY_OK);
    CHECK_EQ(list->entries[0].algo, HASHMONKE_ALGO_SFV);

    CHECK_EQ(list->entries[1].line_number, 5u);
    CHECK(list->entries[1].display_path != nullptr);
    CHECK_EQ(std::string(list->entries[1].display_path), "videos/clip.mp4");
    CHECK(list->entries[1].file_path != nullptr);
    CHECK_EQ(list->entries[1].code, HASHMONKE_ENTRY_OK);

    hashmonke_file_list_free(list);
    std::remove(manifest_path.c_str());
}

struct EventRecord
{
    enum hashmonke_runner_event_type type;
    size_t line_number;
    size_t entry_index;
    std::string path;
    std::string display_path;
    enum hashmonke_hash_code status;
    uint64_t bytes_processed;
    uint64_t file_size;
};

TEST_CASE("TUI Plan: Runner events for start, progress, completion and duplicate paths")
{
    create_test_file("tui_ev1.bin", "content for ev1");
    create_test_file("tui_ev2.bin", "content for ev2");

    std::string crc1 = hash_string(hashmonke_md_crc32(), "content for ev1");
    std::string crc2 = hash_string(hashmonke_md_crc32(), "content for ev2");

    // Manifest contains duplicate paths on different lines
    std::string manifest_content =
        "tui_ev1.bin " + crc1 + "\n" +
        "tui_ev2.bin 00000000\n" +
        "tui_ev1.bin " + crc1 + "\n"; // duplicate path!

    std::string manifest_path = "test_runner_events.sfv";
    create_test_file(manifest_path, manifest_content);

    struct hashmonke_file *manifest = hashmonke_file_init(nullptr, manifest_path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(manifest != nullptr);
    REQUIRE_EQ(manifest->num_entries, 3u);

    std::vector<EventRecord> events;

    auto event_cb = [](const struct hashmonke_runner_event *ev, struct hashmonke_runner_event_context *ctx) {
        auto *records = reinterpret_cast<std::vector<EventRecord> *>(ctx);
        EventRecord rec;
        rec.type = ev->type;
        rec.line_number = ev->line_number;
        rec.entry_index = ev->entry_index;
        rec.path = ev->file_path ? ev->file_path : "";
        rec.display_path = ev->display_path ? ev->display_path : "";
        rec.status = ev->status;
        rec.bytes_processed = ev->bytes_processed;
        rec.file_size = ev->file_size;

        records->push_back(rec);
    };

    struct hashmonke_runner *runner = hashmonke_runner_run_with_events(
        manifest, nullptr, event_cb,
        reinterpret_cast<struct hashmonke_runner_event_context *>(&events),
        1, false);
    REQUIRE(runner != nullptr);

    hashmonke_runner_wait(runner);

    struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
    CHECK(stats.is_finished);
    CHECK_EQ(stats.files_matched, 2u);
    CHECK_EQ(stats.files_failed, 1u);
    CHECK_EQ(stats.total_files_processed, 3u);

    // Verify events were fired
    bool saw_start_0 = false, saw_complete_0 = false;
    bool saw_start_1 = false, saw_complete_1 = false;
    bool saw_start_2 = false, saw_complete_2 = false;

    for (const auto &ev : events)
    {
        if (ev.entry_index == 0)
        {
            CHECK_EQ(ev.line_number, 1u);
            if (ev.type == HASHMONKE_RUNNER_EVENT_START) saw_start_0 = true;
            if (ev.type == HASHMONKE_RUNNER_EVENT_COMPLETE)
            {
                saw_complete_0 = true;
                CHECK_EQ(ev.status, HASHMONKE_HASH_MATCHES);
            }
        }
        else if (ev.entry_index == 1)
        {
            CHECK_EQ(ev.line_number, 2u);
            if (ev.type == HASHMONKE_RUNNER_EVENT_START) saw_start_1 = true;
            if (ev.type == HASHMONKE_RUNNER_EVENT_COMPLETE)
            {
                saw_complete_1 = true;
                CHECK_EQ(ev.status, HASHMONKE_HASH_MISMATCH);
            }
        }
        else if (ev.entry_index == 2)
        {
            CHECK_EQ(ev.line_number, 3u);
            if (ev.type == HASHMONKE_RUNNER_EVENT_START) saw_start_2 = true;
            if (ev.type == HASHMONKE_RUNNER_EVENT_COMPLETE)
            {
                saw_complete_2 = true;
                CHECK_EQ(ev.status, HASHMONKE_HASH_MATCHES);
            }
        }
    }

    CHECK(saw_start_0);
    CHECK(saw_complete_0);
    CHECK(saw_start_1);
    CHECK(saw_complete_1);
    CHECK(saw_start_2);
    CHECK(saw_complete_2);

    hashmonke_runner_free(runner);
    hashmonke_file_free(manifest);

    std::remove("tui_ev1.bin");
    std::remove("tui_ev2.bin");
    std::remove(manifest_path.c_str());
}

TEST_CASE("TUI Plan: Hasher progress callback reports filename and bytes")
{
    std::string test_data(2 * 1024 * 1024, 'A'); // 2 MB
    create_test_file("hasher_progress_test.bin", test_data);

    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    REQUIRE(hasher != nullptr);

    uint8_t expected_buf[sizeof(struct hashmonke_hash) + 20];
    struct hashmonke_hash *expected = (struct hashmonke_hash *)expected_buf;
    expected->algo = HASHMONKE_ALGO_SFV;
    std::string crc = hash_string(hashmonke_md_crc32(), test_data);
    std::vector<uint8_t> bytes = from_hex(crc);
    REQUIRE_EQ(bytes.size(), 4u);
    memcpy(expected->value, bytes.data(), 4);

    struct ProgressRecord
    {
        std::string file;
        uint64_t bytes_hashed;
        uint64_t bytes_total;
    };
    std::vector<ProgressRecord> reports;

    auto progress_cb = [](const struct hashmonke_hasher_progress_event *event,
                          struct hashmonke_hasher_progress_context *ctx) {
        auto *recs = reinterpret_cast<std::vector<ProgressRecord> *>(ctx);
        recs->push_back({event->file_path ? event->file_path : "", event->bytes_hashed, event->bytes_total});
    };

    struct hashmonke_hash_ctrl ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.progress_cb = progress_cb;
    ctrl.progress_context = reinterpret_cast<struct hashmonke_hasher_progress_context *>(&reports);

    enum hashmonke_hash_code code = hashmonke_hasher_hash(
        hasher, "hasher_progress_test.bin", false, expected, &ctrl);

    CHECK_EQ(code, HASHMONKE_HASH_MATCHES);
    CHECK(!reports.empty());
    CHECK_EQ(reports[0].file, "hasher_progress_test.bin");
    CHECK_EQ(reports[0].bytes_total, (uint64_t)test_data.size());

    hashmonke_hasher_free(hasher);
    std::remove("hasher_progress_test.bin");
}

TEST_CASE("TUI Plan: Immediate-mode progress bar and formatting helpers")
{
    // tui_init will return NULL if not a tty (like in CI/pipe), which is expected
    // but we can test that tui_progress_bar safely handles null/edge cases
    tui_progress_bar(nullptr, 50, 100, 20);
    tui_file_row(nullptr, "✓", "\033[32m", "test.txt", "10 MB/s");
    tui_stat_row(nullptr, "Progress", "50%");
    CHECK(true);
}

TEST_CASE("TUI Plan: tui_render helper state and event handling")
{
    std::string path = "test_render_manifest.sfv";
    create_test_file(path, "file1.bin 11111111\nfile2.bin 22222222\n");

    struct hashmonke_file *manifest = hashmonke_file_init(nullptr, path.c_str(), HASHMONKE_FILE_FORMAT_SFV);
    REQUIRE(manifest != nullptr);

    struct tui_render_state *state = tui_render_state_create(manifest, "archive.sfv");
    REQUIRE(state != nullptr);

    // Event updates
    struct hashmonke_runner_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = HASHMONKE_RUNNER_EVENT_START;
    ev.entry_index = 0;
    ev.line_number = 1;
    ev.file_path = "file1.bin";
    tui_render_event_callback(&ev, reinterpret_cast<struct hashmonke_runner_event_context *>(state));

    ev.type = HASHMONKE_RUNNER_EVENT_COMPLETE;
    ev.status = HASHMONKE_HASH_MATCHES;
    tui_render_event_callback(&ev, reinterpret_cast<struct hashmonke_runner_event_context *>(state));

    // Key navigation null safety
    CHECK(tui_render_handle_key(nullptr, state, TUI_KEY_DOWN) == false);

    tui_render_state_free(state);
    hashmonke_file_free(manifest);
    std::remove(path.c_str());
}


