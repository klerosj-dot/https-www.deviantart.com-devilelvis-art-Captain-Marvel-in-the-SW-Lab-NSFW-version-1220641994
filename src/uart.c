#include "uart.h"

static int backend_is_valid(const uart_backend_t *backend)
{
    return backend != NULL &&
           backend->configure != NULL &&
           backend->write != NULL &&
           backend->read != NULL;
}

uart_status_t uart_init(uart_t *uart, const uart_backend_t *backend)
{
    return uart_init_with_baud(uart, backend, UART_DEFAULT_BAUD_RATE);
}

uart_status_t uart_init_with_baud(uart_t *uart,
                                  const uart_backend_t *backend,
                                  uint32_t baud_rate)
{
    const uart_config_t config = {
        .baud_rate = baud_rate,
        .data_bits = 8u,
        .stop_bits = 1u,
        .parity = UART_PARITY_NONE
    };

    return uart_init_with_config(uart, backend, &config);
}

uart_status_t uart_init_with_config(uart_t *uart,
                                    const uart_backend_t *backend,
                                    const uart_config_t *config)
{
    if (uart == NULL || !backend_is_valid(backend) || config == NULL ||
        config->baud_rate == 0u ||
        config->data_bits != 8u ||
        config->stop_bits != 1u ||
        config->parity != UART_PARITY_NONE) {
        return UART_INVALID_ARGUMENT;
    }

    if (backend->configure(backend->context, config->baud_rate) != 0) {
        return UART_IO_ERROR;
    }

    uart->backend = *backend;
    uart->baud_rate = config->baud_rate;
    uart->initialized = 1;
    return UART_OK;
}

static uart_status_t uart_write_raw(uart_t *uart,
                                   const uint8_t *data,
                                   size_t length)
{
    if (uart == NULL || !uart->initialized) {
        return UART_NOT_INITIALIZED;
    }
    if (data == NULL && length != 0u) {
        return UART_INVALID_ARGUMENT;
    }
    if (uart->backend.write(uart->backend.context, data, length) != 0) {
        uart->error_count++;
        return UART_IO_ERROR;
    }
    return UART_OK;
}

uart_status_t uart_write(uart_t *uart, const uint8_t *data, size_t length)
{
    uart_status_t status;

    status = uart_write_raw(uart, data, length);
    if (status != UART_OK) {
        return status;
    }

    uart->write_operations++;
    return UART_OK;
}

uart_status_t uart_write_line(uart_t *uart,
                              const uint8_t *data,
                              size_t length,
                              uart_line_ending_t ending)
{
    static const uint8_t lf[] = {'\n'};
    static const uint8_t crlf[] = {'\r', '\n'};
    const uint8_t *line_ending;
    size_t line_ending_length;
    uart_status_t status;

    if (ending == UART_LINE_FEED) {
        line_ending = lf;
        line_ending_length = sizeof(lf);
    } else if (ending == UART_CARRIAGE_RETURN_LINE_FEED) {
        line_ending = crlf;
        line_ending_length = sizeof(crlf);
    } else {
        return UART_INVALID_ARGUMENT;
    }

    status = uart_write_raw(uart, data, length);
    if (status != UART_OK) {
        return status;
    }
    return uart_write_raw(uart, line_ending, line_ending_length);
}

uart_status_t uart_read(uart_t *uart,
                        uint8_t *data,
                        size_t length,
                        uint32_t timeout_ms)
{
    int backend_status;

    if (uart == NULL || !uart->initialized) {
        return UART_NOT_INITIALIZED;
    }
    if (data == NULL && length != 0u) {
        return UART_INVALID_ARGUMENT;
    }

    backend_status = uart->backend.read(uart->backend.context, data, length, timeout_ms);
    if (backend_status != 0) {
        uart->error_count++;
        return backend_status < 0 ? UART_IO_ERROR : UART_TIMEOUT;
    }
    uart->read_operations++;
    return UART_OK;
}

uart_status_t uart_generate_square_wave(uart_t *uart,
                                         uint32_t frequency_hz,
                                         uint32_t cycles)
{
    if (uart == NULL || !uart->initialized) {
        return UART_NOT_INITIALIZED;
    }
    if (uart->backend.square_wave == NULL ||
        frequency_hz == 0u ||
        cycles == 0u) {
        return UART_INVALID_ARGUMENT;
    }
    if (uart->backend.square_wave(uart->backend.context,
                                  frequency_hz,
                                  cycles) != 0) {
        uart->error_count++;
        return UART_IO_ERROR;
    }
    return UART_OK;
}

uart_status_t uart_get_diagnostics(const uart_t *uart,
                                   uart_diagnostics_t *diagnostics)
{
    if (uart == NULL || diagnostics == NULL) {
        return UART_INVALID_ARGUMENT;
    }
    if (!uart->initialized) {
        return UART_NOT_INITIALIZED;
    }

    diagnostics->baud_rate = uart->baud_rate;
    diagnostics->write_operations = uart->write_operations;
    diagnostics->read_operations = uart->read_operations;
    diagnostics->error_count = uart->error_count;
    return UART_OK;
}

uart_status_t uart_reset_diagnostics(uart_t *uart)
{
    if (uart == NULL) {
        return UART_INVALID_ARGUMENT;
    }
    if (!uart->initialized) {
        return UART_NOT_INITIALIZED;
    }

    uart->write_operations = 0u;
    uart->read_operations = 0u;
    uart->error_count = 0u;
    return UART_OK;
}
