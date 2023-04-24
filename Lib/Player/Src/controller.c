#include "controller.h"
#include "display.h"
#include "files.h"
#include "flac_reader.h"
#include "player.h"

static FileList file_list;
static uint8_t current_file_index = 6;

static const char* get_current_file_path(void) {
    static char path[MAX_FILE_PATH_LENGTH + 1];
    snprintf(path, MAX_FILE_PATH_LENGTH + 1, "%s", file_list.files[current_file_index].path);
    return path;
}

void controller_task(void) {
    set_debug_mode(true);
    log_info("FLAC player starts");

    wait_for_sd_card();

    if (find_FLAC_files("", &file_list) == 1) {
        return;
    }

    initialize_codec();
    start_player(get_current_file_path());

    while (true) {
        update_player();
    }

//    // TODO - extract this code related to the display to a separate module
//    load_screen();
//    render_text();
}
