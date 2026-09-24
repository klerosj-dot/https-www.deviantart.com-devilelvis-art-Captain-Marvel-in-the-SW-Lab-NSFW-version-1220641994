#include "uart.h"

#include <stdio.h>
#include <string.h>

static int demo_configure(void *context, uint32_t baud_rate)
{
    (void)context;
    printf("Configured UART at %u baud (8N1)\n", baud_rate);
    return 0;
}

static int demo_write(void *context, const uint8_t *data, size_t length)
{
    (void)context;
    printf("TX: %.*s\n", (int)length, (const char *)data);
    return 0;
}

static int demo_read(void *context,
                     uint8_t *data,
                     size_t length,
                     uint32_t timeout_ms)
{
    static const char response[] = "OK";
    (void)context;
    (void)timeout_ms;
    if (length > sizeof(response) - 1u) {
        return -1;
    }
    memcpy(data, response, length);
    return 0;
}

static int demo_square_wave(void *context,
                            uint32_t frequency_hz,
                            uint32_t cycles)
{
    (void)context;
    printf("Square wave: %u Hz for %u cycles\n", frequency_hz, cycles);
    return 0;
}

int main(void)
{
    uart_backend_t backend = {
        .configure = demo_configure,
        .write = demo_write,
        .read = demo_read,
        .square_wave = demo_square_wave,
        .context = NULL
    };
    uart_t uart = {0};
    uart_diagnostics_t diagnostics;
    uint8_t response[2] = {0};

    if (uart_init(&uart, &backend) != UART_OK ||
        uart_write(&uart, (const uint8_t *)"PING", 4u) != UART_OK ||
        uart_read(&uart, response, sizeof(response), 100u) != UART_OK ||
        uart_generate_square_wave(&uart, 1000u, 4u) != UART_OK) {
        fprintf(stderr, "UART demo failed\n");
        return 1;
    }

    printf("RX: %.*s\n", (int)sizeof(response), (const char *)response);
    if (uart_get_diagnostics(&uart, &diagnostics) == UART_OK) {
        printf("Diagnostics: baud=%u writes=%u reads=%u errors=%u\n",
               diagnostics.baud_rate,
               diagnostics.write_operations,
               diagnostics.read_operations,
               diagnostics.error_count);
    }
    return 0;
}
