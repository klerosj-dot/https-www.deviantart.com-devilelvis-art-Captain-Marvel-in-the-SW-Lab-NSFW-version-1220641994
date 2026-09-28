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
        .bandwidth_hz = 1000000u,
        .direction = FMCW_SWEEP_ASCENDING,
        .duration_us = 1000u,
        .sample_rate_hz = 10000u,
        .callback = capture_sample,
        .context = &capture
    };

    if (expect(fmcw_generate(&sweep) == FMCW_OK, "FMCW generation succeeds") ||
        expect(capture.calls == 10u, "expected sample count is generated") ||
        expect(capture.sample_count == 10u, "sample count is reported") ||
        expect(capture.first_frequency == 1000000u, "sweep starts at start frequency") ||
        expect(capture.last_frequency == 2000000u, "sweep ends at start plus bandwidth")) {
        return 1;
    }

    sweep.duration_us = 1000u;
    sweep.sample_rate_hz = 1000u;
    capture.calls = 0u;
    if (expect(fmcw_generate(&sweep) == FMCW_OK,
               "single-sample sweep succeeds") ||
        expect(capture.calls == 1u && capture.sample_count == 1u &&
                   capture.first_frequency == 1000000u &&
                   capture.last_frequency == 1000000u,
               "single-sample sweep emits its start frequency once")) {
        return 1;
    }

    sweep.duration_us = 1000u;
    sweep.sample_rate_hz = 10000u;
    sweep.start_frequency_hz = 1500000u;
    sweep.bandwidth_hz = 0u;
    capture.calls = 0u;
    if (expect(fmcw_generate(&sweep) == FMCW_OK,
               "zero-bandwidth sweep succeeds") ||
        expect(capture.calls == 10u && capture.first_frequency == 1500000u &&
                   capture.last_frequency == 1500000u,
               "zero bandwidth emits a constant frequency")) {
        return 1;
    }

    sweep.start_frequency_hz = 2000000u;
    sweep.bandwidth_hz = 500000u;
    sweep.direction = FMCW_SWEEP_DESCENDING;
    capture.calls = 0u;
    if (expect(fmcw_generate(&sweep) == FMCW_OK,
               "descending sweep succeeds") ||
        expect(capture.calls == 10u && capture.first_frequency == 2000000u &&
                   capture.last_frequency == 1500000u,
               "descending sweep ends at start minus bandwidth")) {
        return 1;
    }

    sweep.sample_rate_hz = 0u;
    if (expect(fmcw_generate(&sweep) == FMCW_INVALID_ARGUMENT,
               "zero sample rate is rejected")) {
        return 1;
    }

    sweep.sample_rate_hz = 10000u;
    capture.calls = 0u;
    sweep.start_frequency_hz = UINT32_MAX - 10u;
    sweep.bandwidth_hz = 11u;
    sweep.direction = FMCW_SWEEP_ASCENDING;
    if (expect(fmcw_generate(&sweep) == FMCW_INVALID_ARGUMENT &&
                   capture.calls == 0u,
               "bandwidth that overflows the stop frequency is rejected")) {
        return 1;
    }

    sweep.start_frequency_hz = 10u;
    sweep.bandwidth_hz = 11u;
    sweep.direction = FMCW_SWEEP_DESCENDING;
    return expect(fmcw_generate(&sweep) == FMCW_INVALID_ARGUMENT &&
                      capture.calls == 0u,
                  "bandwidth that underflows the stop frequency is rejected");
}
