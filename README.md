# Platform-independent UART driver

This project provides a small C UART abstraction for embedded targets. The
hardware-specific layer supplies callbacks for configuring, transmitting, and
receiving bytes.

The default configuration is **115200 baud, 8 data bits, no parity, 1 stop
bit (8N1)**. `uart_init_with_config` makes framing explicit and rejects
unsupported settings; this implementation currently accepts only 8N1. The
driver does not access registers or allocate memory, so it can be adapted to
an MCU HAL, bare-metal implementation, or board support package.

## Build

```sh
cmake -S . -B build
cmake --build build
```

The host-side demo can be run with:

```sh
./build/uart_demo
```

It uses mock callbacks to demonstrate initialization, `PING`/`OK` traffic,
and a 1 kHz four-cycle square-wave request. The demo also prints diagnostic
counters for successful reads, writes, and backend errors.

The test suite also verifies intact transmission and reception of a 100-byte
payload.

The tests include serial-input cases that receive byte values `50` and `100`,
plus a check that `0xFF` is preserved as unsigned byte value `255`.
They also cover positive backend receive status as a timeout and negative
backend receive status as an I/O error, including diagnostics accounting.

## FMCW sweep generation

`fmcw_generate` streams a linear frequency-modulated continuous-wave sweep
through a callback. The callback receives the current frequency in hertz,
sample index, and total sample count. The implementation uses 64-bit
intermediate arithmetic to avoid overflow while calculating sample timing and
frequency ramps.

## Diagnostics

Use `uart_get_diagnostics` to snapshot the configured baud rate and operation
counters. `uart_reset_diagnostics` clears the read, write, and error counts
without changing the UART configuration.

## Integrate

Implement `uart_backend_t` callbacks for the target hardware, then initialize
the driver:

```c
uart_t uart;
uart_backend_t backend = {
    .configure = board_uart_configure,
    .write = board_uart_write,
    .read = board_uart_read,
    .context = board_uart_context,
};

uart_status_t status = uart_init(&uart, &backend);
```

Use `uart_init_with_baud` when a baud rate other than 115200 is required.

`uart_write_line` appends either `\n` (LF) or `\r\n` (CRLF) after writing a
payload, which is useful for text-oriented UART protocols.

## Square-wave test signal

The optional `square_wave` backend callback can drive a GPIO or timer-based
test output from the platform layer. Call `uart_generate_square_wave` with a
frequency in hertz and a cycle count. The driver validates the parameters and
forwards them without making hardware assumptions.
