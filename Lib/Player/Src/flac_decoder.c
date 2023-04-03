#include <flac_decoder.h>

Flac* create_flac(FIL* input) {
    Flac* flac = (Flac *) calloc(1, sizeof(Flac));
}

void destroy_flac(Flac* flac) {
    if (flac != NULL) {
        if (flac->decoder != NULL) {
            FLAC__stream_decoder_delete(flac->decoder);
        }
        free(flac);
    }
}