#pragma once
#include <stdint.h>

uint32_t crc32_init(void);
uint32_t crc32_update(uint32_t crc, const uint8_t *buf, uint32_t len);
uint32_t crc32_final(uint32_t crc);
uint32_t crc32_compute(const uint8_t *buf, uint32_t len);
