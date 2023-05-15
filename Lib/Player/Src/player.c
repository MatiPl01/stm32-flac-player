#include <assert.h>
#include "player.h"
#include "files.h"

static uint8_t audio_buffer[AUDIO_BUFFER_SIZE];
static uint8_t audio_buffer_state = BUFFER_OFFSET_NONE;
static unsigned last_audio_buffer_state_change_time = 0;

static PlayerState player_state = STOPPED;
static uint64_t samples_played = 0;

static FIL current_audio_file;
static Flac *flac;
static FlacReader *flac_reader;
static FlacMetaData flac_metadata;

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
    audio_buffer_state = BUFFER_OFFSET_HALF;
    unsigned t = osKernelSysTick();
    log_debug("[%u] TransferredFirstHalf (%u)\n", t, t - last_audio_buffer_state_change_time);
    last_audio_buffer_state_change_time = t;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
    audio_buffer_state = BUFFER_OFFSET_FULL;
    unsigned t = osKernelSysTick();
    log_debug("[%u] TransferredSecondHalf (%u)\n", t, t - last_audio_buffer_state_change_time);
    last_audio_buffer_state_change_time = t;
}

void initialize_codec(void) {
    log_info("Initializing audio codec");
    if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE1, 10, AUDIO_FREQUENCY_44K) == 0) {
        log_success("Audio codec was successfully initialized");
    } else {
        log_error("Failed to initialize audio codec");
    }
    BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
}

void start_player(const char* file_path) {
    log_info("Playing file %s", file_path);

    assert(player_state == STOPPED);

    player_state = PLAYING;
    open_file(file_path, &current_audio_file);

    log_info("Creating FLAC reader");
    flac = create_flac(&current_audio_file);
    flac_reader = create_flac_reader(flac);

    log_info("Reading FLAC metadata");
    // TODO - handle errors instead of returning from a function
    if (read_metadata(flac, &flac_metadata) == 1) return;

    log_info("Reading FLAC file into buffer");
    // Fill the first half of the buffer
    unsigned bytes_to_read = AUDIO_BUFFER_SIZE / 2;
    unsigned bytes_read = read_flac(flac_reader, audio_buffer, bytes_to_read);
    log_info(">>>> Read %d bytes", bytes_read); // TODO - remove this line
    if (bytes_read < bytes_to_read) {
        log_info("Reached end of file");
        stop_player();
        return;
    }

    audio_buffer_state = BUFFER_OFFSET_HALF;
    last_audio_buffer_state_change_time = HAL_GetTick();

    log_info("Starting playing audio file");
    BSP_AUDIO_OUT_Play(audio_buffer, AUDIO_BUFFER_SIZE);
    BSP_AUDIO_OUT_Resume();

    log_info("Started playing");
}

void pause_player(void) {
    log_info("Pausing player");

    assert(player_state == PLAYING);

    BSP_AUDIO_OUT_Pause();
    player_state = PAUSED;
}

void resume_player(void) {
    log_info("Resuming player");

    assert(player_state == PAUSED);

    BSP_AUDIO_OUT_Resume();
    player_state = PLAYING;
}

void stop_player(void) {
    log_info("Stopping player");

    assert(player_state == PLAYING || player_state == PAUSED);

    log_info("Stopping audio codec");
    BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
    player_state = STOPPED;

    log_info("Destroying flac reader");
    free_flac_reader(flac_reader);
    destroy_flac(flac);

    log_info("Closing file");
    f_close(&current_audio_file);
    samples_played = 0;

    log_info("Stopped playing");
}

static BufferState get_buffer_state() {
    BufferState buffer_state = audio_buffer_state;
    audio_buffer_state = BUFFER_OFFSET_NONE;
    return buffer_state;
}

void update_player(void) {
    if (player_state == PLAYING) {
        BufferState buffer_state = get_buffer_state();
        if (buffer_state) {
            uint32_t offset;
            if (buffer_state == BUFFER_OFFSET_HALF) {
                offset = 0;
            } else {
                offset = AUDIO_BUFFER_SIZE / 2;
            }

            unsigned bytes_read = read_flac(flac_reader, &audio_buffer[offset], AUDIO_BUFFER_SIZE / 2);
            samples_played += bytes_read / flac_metadata.channels / (flac_metadata.bits_per_sample / 8);

            if (bytes_read < AUDIO_BUFFER_SIZE / 2) {
                log_info("Stop at EOF");
                stop_player();
            }
        }
    }
}

double get_playing_progress(void) {
    if (flac_metadata.total_samples == 0) {
        return 0;
    }
    double progress = (double) samples_played / flac_metadata.total_samples;
    if (progress > 1) {
        progress = 1;
    }
    return progress;
}
