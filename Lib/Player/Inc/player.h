#ifndef STM32_FLAC_PLAYER_PLAYER_H
#define STM32_FLAC_PLAYER_PLAYER_H

#include "flac_reader.h"
#include "stm32746g_discovery_audio.h"

typedef enum {
    STOPPED,
    PLAYING,
    PAUSED
} PlayerState;

typedef enum {
    BUFFER_OFFSET_NONE = 0,
    BUFFER_OFFSET_HALF,
    BUFFER_OFFSET_FULL,
} BufferState;

#define AUDIO_BUFFER_SIZE 32768

void initialize_codec();
void start_player(const char* file_path);
void pause_player();
void resume_player();
void stop_player();
void update_player();


#endif //STM32_FLAC_PLAYER_PLAYER_H
