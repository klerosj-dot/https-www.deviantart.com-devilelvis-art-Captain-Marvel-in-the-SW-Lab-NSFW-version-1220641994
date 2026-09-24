#include "uart.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t configured_baud;
    uint8_t written[128];
    size_t written_length;
    uint8_t received[128];
    size_t received_length;
    uint32_t read_timeout;
    uint32_t square_wave_frequency;
    uint32_t square_wave_cycles;
} mock_uart_t;

static int mock_configure(void *context, uint32_t baud_rate)
{
    mock_uart_t *mock = context;
    mock->configured_baud = baud_rate;
    return 0;
}

static int mock_write(void *context, const uint8_t *data, size_t length)
{
    mock_uart_t *mock = context;
    if (length > sizeof(mock->written)) {
        return -1;
    }
    memcpy(mock->written, data, length);
    mock->written_length = length;
    return 0;
}

static int mock_read(void *context, uint8_t *data, size_t length, uint32_t timeout_ms)
{
    mock_uart_t *mock = context;
    if (length > mock->received_length) {
        return -1;
    }
    memcpy(data, mock->received, length);
    mock->read_timeout = timeout_ms;
    return 0;
}

static int mock_square_wave(void *context,
                            uint32_t frequency_hz,
                            uint32_t cycles)
{
    mock_uart_t *mock = context;
    mock->square_wave_frequency = frequency_hz;
    mock->square_wave_cycles = cycles;
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

static uart_backend_t mock_backend(mock_uart_t *mock)
{
    uart_backend_t backend = {
        .configure = mock_configure,
        .write = mock_write,
        .read = mock_read,
        .square_wave = mock_square_wave,
        .context = mock
    };
    return backend;
}

static int test_default_initialization(void)
{
    mock_uart_t mock = {0};
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};

    if (expect(uart_init(&uart, &backend) == UART_OK, "default init succeeds") ||
        expect(mock.configured_baud == UART_DEFAULT_BAUD_RATE, "default baud is 115200") ||
        expect(uart.baud_rate == UART_DEFAULT_BAUD_RATE, "UART stores default baud")) {
        return 1;
    }
    return 0;
}

static int test_custom_baud_and_io(void)
{
    static const uint8_t message[] = "hello";
    mock_uart_t mock = {
        .received = {'o', 'k'},
        .received_length = 2
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uart_diagnostics_t diagnostics;
    uint8_t received[2] = {0};

    if (expect(uart_init_with_baud(&uart, &backend, 9600u) == UART_OK,
               "custom init succeeds") ||
        expect(mock.configured_baud == 9600u, "custom baud is forwarded") ||
        expect(uart_write(&uart, message, sizeof(message) - 1u) == UART_OK,
               "write succeeds") ||
        expect(mock.written_length == sizeof(message) - 1u, "write length is forwarded") ||
        expect(memcmp(mock.written, message, sizeof(message) - 1u) == 0,
               "write data is forwarded") ||
        expect(uart_write_line(&uart, (const uint8_t *)"LF", 2u,
                               UART_LINE_FEED) == UART_OK,
               "LF line write succeeds") ||
        expect(mock.written_length == 1u && mock.written[0] == '\n',
               "LF ending is forwarded") ||
        expect(uart_write_line(&uart, (const uint8_t *)"CRLF", 4u,
                               UART_CARRIAGE_RETURN_LINE_FEED) == UART_OK,
               "CRLF line write succeeds") ||
        expect(mock.written_length == 2u &&
                   mock.written[0] == '\r' && mock.written[1] == '\n',
               "CRLF ending is forwarded") ||
        expect(uart_read(&uart, received, sizeof(received), 25u) == UART_OK,
               "read succeeds") ||
        expect(memcmp(received, mock.received, sizeof(received)) == 0,
               "read data is forwarded") ||
        expect(mock.read_timeout == 25u, "read timeout is forwarded")) {
        return 1;
    }
    if (expect(uart_generate_square_wave(&uart, 1000u, 4u) == UART_OK,
               "square-wave generation succeeds") ||
        expect(mock.square_wave_frequency == 1000u, "square-wave frequency is forwarded") ||
        expect(mock.square_wave_cycles == 4u, "square-wave cycles are forwarded") ||
        expect(uart_get_diagnostics(&uart, &diagnostics) == UART_OK,
               "diagnostics snapshot succeeds") ||
        expect(diagnostics.baud_rate == 9600u, "diagnostics report baud") ||
        expect(diagnostics.write_operations == 1u, "diagnostics count writes") ||
        expect(diagnostics.read_operations == 1u, "diagnostics count reads") ||
        expect(diagnostics.error_count == 0u, "diagnostics count no errors") ||
        expect(uart_reset_diagnostics(&uart) == UART_OK,
               "diagnostics reset succeeds") ||
        expect(uart.write_operations == 0u && uart.read_operations == 0u &&
                   uart.error_count == 0u,
               "diagnostics counters reset")) {
        return 1;
    }
    return 0;
}

static int test_100_byte_transfer(void)
{
    mock_uart_t mock = {0};
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uint8_t transmit[100];
    uint8_t receive[100];
    size_t index;

    for (index = 0; index < sizeof(transmit); index++) {
        transmit[index] = (uint8_t)index;
        mock.received[index] = (uint8_t)(sizeof(receive) - index - 1u);
    }
    mock.received_length = sizeof(mock.received);

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "100-byte test initializes UART") ||
        expect(uart_write(&uart, transmit, sizeof(transmit)) == UART_OK,
               "100-byte write succeeds") ||
        expect(mock.written_length == sizeof(transmit),
               "100-byte write length is forwarded") ||
        expect(memcmp(mock.written, transmit, sizeof(transmit)) == 0,
               "100-byte write data is intact") ||
        expect(uart_read(&uart, receive, sizeof(receive), 100u) == UART_OK,
               "100-byte read succeeds") ||
        expect(memcmp(receive, mock.received, sizeof(receive)) == 0,
               "100-byte read data is intact")) {
        return 1;
    }
    return 0;
}

static int test_serial_input_value_100(void)
{
    mock_uart_t mock = {
        .received = {100u},
        .received_length = 1u
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uint8_t value = 0u;

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "serial-input test initializes UART") ||
        expect(uart_read(&uart, &value, 1u, 100u) == UART_OK,
               "serial-input value read succeeds") ||
        expect(value == 100u, "serial-input value is 100")) {
        return 1;
    }
    return 0;
}

static int test_invalid_usage(void)
{
    uart_t uart = {0};
    uint8_t byte = 0;

    if (expect(uart_write(&uart, &byte, 1u) == UART_NOT_INITIALIZED,
               "write rejects uninitialized UART") ||
        expect(uart_read(&uart, &byte, 1u, 0u) == UART_NOT_INITIALIZED,
               "read rejects uninitialized UART") ||
        expect(uart_generate_square_wave(&uart, 1000u, 1u) == UART_NOT_INITIALIZED,
               "square wave rejects uninitialized UART") ||
        expect(uart_init(NULL, NULL) == UART_INVALID_ARGUMENT,
               "init rejects invalid arguments")) {
        return 1;
    }
    return 0;
}

int main(void)
{
    return test_default_initialization() ||
           test_custom_baud_and_io() ||
           test_100_byte_transfer() ||
           test_serial_input_value_100() ||
           test_invalid_usage();
}
