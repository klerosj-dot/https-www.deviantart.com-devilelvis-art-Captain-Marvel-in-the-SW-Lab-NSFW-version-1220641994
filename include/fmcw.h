#ifndef FMCW_H
#define FMCW_H

#include <stdint.h>

typedef enum {
    FMCW_OK = 0,
    FMCW_INVALID_ARGUMENT = -1,
    FMCW_CALLBACK_ERROR = -2
} fmcw_status_t;

typedef int (*fmcw_sample_callback_t)(void *context,
                                      uint32_t frequency_hz,
                                      uint32_t sample_index,
                                      uint32_t sample_count);

typedef struct {
    uint32_t start_frequency_hz;
    uint32_t stop_frequency_hz;
    uint32_t duration_us;
    uint32_t sample_rate_hz;
    fmcw_sample_callback_t callback;
    void *context;
} fmcw_sweep_t;

fmcw_status_t fmcw_generate(const fmcw_sweep_t *sweep);

#endif
