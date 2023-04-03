#ifndef STM32_FLAC_PLAYER_FILES_H
#define STM32_FLAC_PLAYER_FILES_H

#define MAX_FILE_PATH_LENGTH 100
#define MAX_NUMBER_OF_FILES 25

typedef struct {
    char path[MAX_FILE_PATH_LENGTH + 1];
} File;

typedef struct {
    File files[MAX_NUMBER_OF_FILES];
    int count;
} FileList;

void wait_for_sd_card(void);

int find_FLAC_files(const char *dir_path, FileList *file_list);

#endif //STM32_FLAC_PLAYER_FILES_H
