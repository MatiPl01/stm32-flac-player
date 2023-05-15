#include <string.h>
#include "controller.h"
#include "display.h"
#include "files.h"
#include "flac_reader.h"
#include "player.h"

static FileList file_list;
static uint8_t current_file_index = 6;

static const char *get_current_file_path(void) {
    static char path[MAX_FILE_PATH_LENGTH + 1];
    snprintf(path, MAX_FILE_PATH_LENGTH + 1, "%s", file_list.files[current_file_index].path);
    return path;
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

    start_player(get_current_file_path());

    while (true) {
        render_track_screen(get_current_file_path(), "Author", 3, 0, get_playing_progress(), 182.42, true);
        update_player();
    }
}
