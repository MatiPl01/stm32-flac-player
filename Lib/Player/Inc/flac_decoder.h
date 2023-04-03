#ifndef STM32_FLAC_PLAYER_FLAC_DECODER_H
#define STM32_FLAC_PLAYER_FLAC_DECODER_H

#include <stdint.h>
#include "FLAC/stream_decoder.h"
#include "ff.h"
#include <stdlib.h>

typedef struct {
    uint64_t total_samples;
    unsigned bits_per_sample;
    unsigned sample_rate;
    unsigned channels;
} FlacMetaData;

typedef struct {
    int samples;
    int size;
    uint8_t* buffer;
} FlacFrame;

typedef struct {
    FLAC__StreamDecoder* decoder;
    FlacMetaData meta_data;
    FlacFrame* frames;
    FIL file;
} Flac;

#endif //STM32_FLAC_PLAYER_FLAC_DECODER_H
