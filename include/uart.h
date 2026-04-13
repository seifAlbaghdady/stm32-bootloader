#pragma once
#include <stdint.h>
#include <stdbool.h>

void uart_init(uint32_t baud);
void uart_send_byte(uint8_t b);
void uart_send(const uint8_t *buf, uint32_t len);
uint8_t uart_recv_byte(void);
bool uart_recv_timeout(uint8_t *out, uint32_t timeout_ms);
void uart_send_str(const char *s);
