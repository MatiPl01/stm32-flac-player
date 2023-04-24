#include <string.h>
#include "files.h"
#include "ff.h"
#include "logger.h"

int find_FLAC_files(const char *dir_path, FileList *file_list) {
    log_info("Searching for FLAC files in %s", dir_path);
    file_list->count = 0;

    DIR dir;
    if (f_opendir(&dir, dir_path) != FR_OK) {
        log_error("Failed to open directory %s", dir_path);
        return 1;
    }

    while (file_list->count < MAX_NUMBER_OF_FILES) {
        FILINFO file_info;
        if (f_readdir(&dir, &file_info) != FR_OK) {
            log_error("Failed to read directory %s", dir_path);
            return 1;
        }

        if (file_info.fname[0] == 0) {
            break;
        }

        if (file_info.fattrib & AM_DIR) {
            if (find_FLAC_files(file_info.fname, file_list) == 1) {
                return 1;
            }
        } else {
            if (file_info.fattrib & AM_ARC) {
                // check if filename has .flac extension
                char *extension = strrchr(file_info.fname, '.');
                if (extension == NULL || strcmp(extension, ".flac") != 0) {
                    continue;
                }

                log_info("Found file %s", file_info.fname);
                strcpy(file_list->files[file_list->count].path, dir_path);
                strcat(file_list->files[file_list->count].path, "/");
                strcat(file_list->files[file_list->count].path, file_info.fname);
                file_list->count++;
            }
        }
    }

    if (file_list->count != 0) {
        log_success("Found %d FLAC files in %s", file_list->count, dir_path);
    }
    f_closedir(&dir);

    return 0;
}

int open_file(const char *file_path, FIL *file) {
    log_info("Opening file %s", file_path);

    if (f_open(file, file_path, FA_READ) != FR_OK) {
        log_error("Failed to open file %s", file_path);
        return 1;
    }

    log_success("File %s opened", file_path);
    return 0;
}

void wait_for_sd_card(void) {
    log_info("Waiting for SD card to be inserted");

    while (BSP_SD_IsDetected() != SD_PRESENT) {
        HAL_Delay(100);
    }

    log_success("SD card detected");
}
