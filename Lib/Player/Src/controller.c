#include "controller.h"
#include "display.h"
#include "logger.h"
#include "files.h"

static FileList file_list;

void controller_task(void) {
    log_info("FLAC player starts");

//    wait_for_sd_card();

    // TODO - debug this function
//    if (find_FLAC_files("0:", &file_list) == 1) {
//        return;
//    }

    // TODO - extract this code related to the display to a separate module
    load_screen();
    render_text();
}
