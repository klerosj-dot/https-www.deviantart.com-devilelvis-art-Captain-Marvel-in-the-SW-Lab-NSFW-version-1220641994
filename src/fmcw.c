#include "fmcw.h"

fmcw_status_t fmcw_generate(const fmcw_sweep_t *sweep)
{
    uint64_t sample_count;
    uint32_t index;

    if (sweep == NULL ||
        sweep->callback == NULL ||
        sweep->sample_rate_hz == 0u ||
        sweep->duration_us == 0u) {
        return FMCW_INVALID_ARGUMENT;
    }

    sample_count = ((uint64_t)sweep->sample_rate_hz *
                    (uint64_t)sweep->duration_us) / 1000000u;
    if (sample_count == 0u || sample_count > UINT32_MAX) {
        return FMCW_INVALID_ARGUMENT;
    }

    if (sample_count == 1u) {
        if (sweep->callback(sweep->context,
                            sweep->start_frequency_hz,
                            0u,
                            1u) != 0) {
            return FMCW_CALLBACK_ERROR;
        }
        return FMCW_OK;
    }

    for (index = 0u; index < (uint32_t)sample_count; index++) {
        int64_t start_frequency = (int64_t)sweep->start_frequency_hz;
        int64_t stop_frequency = (int64_t)sweep->stop_frequency_hz;
        int64_t frequency_delta = stop_frequency - start_frequency;
        int64_t frequency = start_frequency +
                            (frequency_delta * (int64_t)index) /
                                ((int64_t)sample_count - 1ll);

        if (sweep->callback(sweep->context,
                            (uint32_t)frequency,
                            index,
                            (uint32_t)sample_count) != 0) {
            return FMCW_CALLBACK_ERROR;
        }
    }

    return FMCW_OK;
}
