#include "controller.h"
#include "display.h"
#include "files.h"
#include "flac_reader.h"
#include "player.h"
#include "utils.h"

static FileList file_list;
static uint8_t current_file_index = 0;

static char track_author[64];
static char track_name[128];

static const char *get_current_file_path(void) {
    static char path[MAX_FILE_PATH_LENGTH + 1];
    snprintf(path, MAX_FILE_PATH_LENGTH + 1, "%s", file_list.files[current_file_index].path);
    return path;
}

static void play_next() {
    if (get_player_state() != STOPPED) {
        stop_player();
    }

    current_file_index = (current_file_index + 1) % file_list.count;
    log_debug("Current file index: %d", current_file_index);
}

static void play_previous() {
    if (get_player_state() != STOPPED) {
        stop_player();
    }
    if (get_playing_progress() <= 0.1) {
        current_file_index = (file_list.count + current_file_index - 1) % file_list.count;
        log_debug("Current file index: %d", current_file_index);
    }
}

static void start() {
    PlayerState state = get_player_state();
    if (state == STOPPED) {
        start_player(get_current_file_path());
    } else if (state == PAUSED) {
        resume_player();
    }
}

static void pause() {
    pause_player();
}

void update_track_info(void) {
    get_author_and_track_name(get_current_file_path(), track_author, track_name);
}

void controller_task(void) {
    set_debug_mode(false);
    log_info("FLAC player starts");

    initialize_screen();
    render_info_screen("Initialization", "Waiting for SD card...");

    wait_for_sd_card();

    render_info_screen("Initialization", "Searching for FLAC files...");
    if (find_FLAC_files("", &file_list) == 1) {
        return;
    }

    render_info_screen("Initialization", "Setting up codec...");
    initialize_codec();
    update_track_info();

    while (true) {
        handle_touch();
        render_track_screen(track_name, track_author, 3, 0, get_playing_progress(), 182.42, get_player_state() == PLAYING);
        if (is_next_button_active()) {
            update_track_info();
            play_next();
        } else if (is_back_button_active()) {
            update_track_info();
            play_previous();
        } else if (is_play_button_active()) {
            start();
        } else if (is_pause_button_active()) {
            pause();
        }
        update_player();
    }
}
