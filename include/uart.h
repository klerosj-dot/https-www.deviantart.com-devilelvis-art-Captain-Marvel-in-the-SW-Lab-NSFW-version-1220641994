#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>

#define UART_DEFAULT_BAUD_RATE 115200u

typedef enum {
    UART_OK = 0,
    UART_INVALID_ARGUMENT = -1,
    UART_NOT_INITIALIZED = -2,
    UART_TIMEOUT = -3,
    UART_IO_ERROR = -4
} uart_status_t;

typedef enum {
    UART_LINE_FEED = 0,
    UART_CARRIAGE_RETURN_LINE_FEED = 1
} uart_line_ending_t;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD = 2
} uart_parity_t;

typedef struct {
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uart_parity_t parity;
} uart_config_t;

typedef int (*uart_square_wave_fn)(void *context,
                                   uint32_t frequency_hz,
                                   uint32_t cycles);

typedef struct {
    int (*configure)(void *context, uint32_t baud_rate);
    int (*write)(void *context, const uint8_t *data, size_t length);
    int (*read)(void *context, uint8_t *data, size_t length, uint32_t timeout_ms);
    uart_square_wave_fn square_wave;
    void *context;
} uart_backend_t;

typedef struct {
    uart_backend_t backend;
    uint32_t baud_rate;
    int initialized;
    uint32_t write_operations;
    uint32_t read_operations;
    uint32_t error_count;
} uart_t;

typedef struct {
    uint32_t baud_rate;
    uint32_t write_operations;
    uint32_t read_operations;
    uint32_t error_count;
} uart_diagnostics_t;

uart_status_t uart_init(uart_t *uart, const uart_backend_t *backend);
uart_status_t uart_init_with_baud(uart_t *uart,
                                  const uart_backend_t *backend,
                                  uint32_t baud_rate);
uart_status_t uart_init_with_config(uart_t *uart,
                                    const uart_backend_t *backend,
                                    const uart_config_t *config);
uart_status_t uart_write(uart_t *uart, const uint8_t *data, size_t length);
uart_status_t uart_write_line(uart_t *uart,
                              const uint8_t *data,
                              size_t length,
                              uart_line_ending_t ending);
uart_status_t uart_read(uart_t *uart,
                        uint8_t *data,
                        size_t length,
                        uint32_t timeout_ms);
uart_status_t uart_generate_square_wave(uart_t *uart,
                                         uint32_t frequency_hz,
                                         uint32_t cycles);
uart_status_t uart_get_diagnostics(const uart_t *uart,
                                   uart_diagnostics_t *diagnostics);
uart_status_t uart_reset_diagnostics(uart_t *uart);

#endif
