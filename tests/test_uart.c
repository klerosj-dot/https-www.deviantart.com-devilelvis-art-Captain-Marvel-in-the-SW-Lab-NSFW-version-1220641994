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
    int read_result;
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
    if (mock->read_result != 0) {
        return mock->read_result;
    }
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

static int test_invalid_framing_configuration(void)
{
    mock_uart_t mock = {0};
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uart_config_t config = {
        .baud_rate = UART_DEFAULT_BAUD_RATE,
        .data_bits = 7u,
        .stop_bits = 1u,
        .parity = UART_PARITY_NONE
    };

    if (expect(uart_init_with_config(&uart, &backend, &config) ==
                   UART_INVALID_ARGUMENT,
               "7 data bits are rejected") ||
        expect(mock.configured_baud == 0u,
               "invalid framing does not configure backend")) {
        return 1;
    }

    config.data_bits = 8u;
    config.parity = UART_PARITY_EVEN;
    if (expect(uart_init_with_config(&uart, &backend, &config) ==
                   UART_INVALID_ARGUMENT,
               "even parity is rejected") ||
        expect(mock.configured_baud == 0u,
               "invalid parity does not configure backend")) {
        return 1;
    }

    config.parity = UART_PARITY_NONE;
    config.stop_bits = 2u;
    if (expect(uart_init_with_config(&uart, &backend, &config) ==
                   UART_INVALID_ARGUMENT,
               "two stop bits are rejected") ||
        expect(mock.configured_baud == 0u,
               "invalid stop bits do not configure backend")) {
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

static int test_serial_input_value_50(void)
{
    mock_uart_t mock = {
        .received = {50u},
        .received_length = 1u
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uint8_t value = 0u;

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "serial-input 50 test initializes UART") ||
        expect(uart_read(&uart, &value, 1u, 100u) == UART_OK,
               "serial-input value 50 read succeeds") ||
        expect(value == 50u, "serial-input value is 50")) {
        return 1;
    }
    return 0;
}

static int test_unsigned_byte_receive(void)
{
    mock_uart_t mock = {
        .received = {UINT8_MAX},
        .received_length = 1u
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uint8_t value = 0u;

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "unsigned-byte test initializes UART") ||
        expect(uart_read(&uart, &value, 1u, 100u) == UART_OK,
               "unsigned-byte receive succeeds") ||
        expect(value == UINT8_MAX,
               "0xFF is preserved as unsigned byte value 255")) {
        return 1;
    }
    return 0;
}

static int test_receive_failures(void)
{
    mock_uart_t mock = {
        .received = {'O'},
        .received_length = 1u,
        .read_result = 1
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uart_diagnostics_t diagnostics;
    uint8_t received = 0u;

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "receive-failure test initializes UART") ||
        expect(uart_read(&uart, &received, 1u, 10u) == UART_TIMEOUT,
               "positive backend read status maps to timeout") ||
        expect(uart_get_diagnostics(&uart, &diagnostics) == UART_OK,
               "diagnostics available after timeout") ||
        expect(diagnostics.read_operations == 0u && diagnostics.error_count == 1u,
               "timeout increments errors but not successful reads")) {
        return 1;
    }

    mock.read_result = -1;
    if (expect(uart_read(&uart, &received, 1u, 10u) == UART_IO_ERROR,
               "negative backend read status maps to I/O error") ||
        expect(uart_get_diagnostics(&uart, &diagnostics) == UART_OK,
               "diagnostics available after I/O error") ||
        expect(diagnostics.read_operations == 0u && diagnostics.error_count == 2u,
               "I/O error increments errors but not successful reads")) {
        return 1;
    }
    return 0;
}

static int test_quick_integration(void)
{
    static const uint8_t payload[] = "READY";
    uint8_t receive[5] = {0};
    mock_uart_t mock = {
        .received = {'R', 'E', 'A', 'D', 'Y'},
        .received_length = sizeof(receive)
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uart_diagnostics_t diagnostics;

    if (expect(uart_init_with_baud(&uart, &backend, 115200u) == UART_OK,
               "quick integration initializes UART") ||
        expect(uart_write(&uart, payload, sizeof(payload) - 1u) == UART_OK,
               "quick integration write succeeds") ||
        expect(uart_read(&uart, receive, sizeof(receive), 25u) == UART_OK,
               "quick integration read succeeds") ||
        expect(memcmp(receive, mock.received, sizeof(receive)) == 0,
               "quick integration payload matches") ||
        expect(uart_generate_square_wave(&uart, 2000u, 2u) == UART_OK,
               "quick integration square-wave succeeds") ||
        expect(uart_get_diagnostics(&uart, &diagnostics) == UART_OK,
               "quick integration diagnostics succeed") ||
        expect(diagnostics.write_operations == 1u &&
                   diagnostics.read_operations == 1u &&
                   diagnostics.error_count == 0u,
               "quick integration diagnostics are sane")) {
        return 1;
    }
    return 0;
}

static int test_square_wave_signal(void)
{
    mock_uart_t mock = {0};
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "square-wave test initializes UART") ||
        expect(uart_generate_square_wave(&uart, 2500u, 5u) == UART_OK,
               "square-wave signal generation succeeds") ||
        expect(mock.square_wave_frequency == 2500u,
               "square-wave frequency is set") ||
        expect(mock.square_wave_cycles == 5u,
               "square-wave cycle count is set")) {
        return 1;
    }
    return 0;
}

static int test_four_character_string_receive(void)
{
    static const uint8_t expected[] = "PING";
    mock_uart_t mock = {
        .received = {'P', 'I', 'N', 'G'},
        .received_length = sizeof(expected) - 1u
    };
    uart_backend_t backend = mock_backend(&mock);
    uart_t uart = {0};
    uint8_t receive[sizeof(expected) - 1u] = {0};

    if (expect(uart_init(&uart, &backend) == UART_OK,
               "4-char string init succeeds") ||
        expect(uart_read(&uart, receive, sizeof(receive), 25u) == UART_OK,
               "4-char string receive succeeds") ||
        expect(memcmp(receive, expected, sizeof(expected) - 1u) == 0,
               "4-char string payload matches")) {
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
           test_invalid_framing_configuration() ||
           test_custom_baud_and_io() ||
           test_100_byte_transfer() ||
           test_serial_input_value_100() ||
           test_serial_input_value_50() ||
           test_unsigned_byte_receive() ||
           test_quick_integration() ||
           test_square_wave_signal() ||
           test_four_character_string_receive() ||
           test_receive_failures() ||
           test_invalid_usage();
}
