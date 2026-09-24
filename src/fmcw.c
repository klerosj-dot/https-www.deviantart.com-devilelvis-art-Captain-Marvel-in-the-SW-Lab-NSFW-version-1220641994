#include "fmcw.h"

fmcw_status_t fmcw_generate(const fmcw_sweep_t *sweep)
{
    uint64_t sample_count;
    uint32_t index;

    if (sweep == NULL ||
        sweep->callback == NULL ||
        sweep->sample_rate_hz == 0u ||
        sweep->duration_us == 0u ||
        sweep->start_frequency_hz > sweep->stop_frequency_hz) {
        return FMCW_INVALID_ARGUMENT;
    }

    sample_count = ((uint64_t)sweep->sample_rate_hz *
                    (uint64_t)sweep->duration_us) / 1000000u;
    if (sample_count < 2u || sample_count > UINT32_MAX) {
        return FMCW_INVALID_ARGUMENT;
    }

    for (index = 0u; index < (uint32_t)sample_count; index++) {
        uint64_t frequency_delta = (uint64_t)sweep->stop_frequency_hz -
                                   (uint64_t)sweep->start_frequency_hz;
        uint64_t frequency = (uint64_t)sweep->start_frequency_hz +
                             (frequency_delta * index) /
                                 ((uint32_t)sample_count - 1u);

        if (sweep->callback(sweep->context,
                            (uint32_t)frequency,
                            index,
                            (uint32_t)sample_count) != 0) {
            return FMCW_CALLBACK_ERROR;
        }
    }

    return FMCW_OK;
}
