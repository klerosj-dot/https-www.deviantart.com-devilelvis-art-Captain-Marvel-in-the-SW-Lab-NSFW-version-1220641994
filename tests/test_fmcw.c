#include "fmcw.h"

#include <stdio.h>

typedef struct {
    uint32_t first_frequency;
    uint32_t last_frequency;
    uint32_t sample_count;
    uint32_t calls;
} sweep_capture_t;

static int capture_sample(void *context,
                          uint32_t frequency_hz,
                          uint32_t sample_index,
                          uint32_t sample_count)
{
    sweep_capture_t *capture = context;
    if (sample_index == 0u) {
        capture->first_frequency = frequency_hz;
    }
    capture->last_frequency = frequency_hz;
    capture->sample_count = sample_count;
    capture->calls++;
    return 0;
}

static int expect(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 1;
    }
    return 0;
}

int main(void)
{
    sweep_capture_t capture = {0};
    fmcw_sweep_t sweep = {
        .start_frequency_hz = 1000000u,
        .stop_frequency_hz = 2000000u,
        .duration_us = 1000u,
        .sample_rate_hz = 10000u,
        .callback = capture_sample,
        .context = &capture
    };

    if (expect(fmcw_generate(&sweep) == FMCW_OK, "FMCW generation succeeds") ||
        expect(capture.calls == 10u, "expected sample count is generated") ||
        expect(capture.sample_count == 10u, "sample count is reported") ||
        expect(capture.first_frequency == 1000000u, "sweep starts at start frequency") ||
        expect(capture.last_frequency == 2000000u, "sweep ends at stop frequency")) {
        return 1;
    }

    sweep.sample_rate_hz = 0u;
    return expect(fmcw_generate(&sweep) == FMCW_INVALID_ARGUMENT,
                  "zero sample rate is rejected");
}
