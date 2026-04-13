#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FLASH_BASE_ADDR 0x08000000UL
#define APP_START_ADDR 0x0800C000UL
#define BOOT_FLAG_ADDR 0x08008000UL
#define BOOT_FLAG_UPDATE 0xDEADBEEFUL
#define BOOT_FLAG_CLEAR 0xFFFFFFFFUL

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERROR = 1,
    FLASH_BUSY = 2,
} FlashStatus;

FlashStatus flash_unlock(void);
void flash_lock(void);
FlashStatus flash_erase_sector(uint8_t sector);
FlashStatus flash_write_word(uint32_t addr, uint32_t data);
FlashStatus flash_write_buffer(uint32_t addr, const uint8_t *buf, uint32_t len);
uint32_t flash_read_word(uint32_t addr);
bool flash_verify(uint32_t addr, const uint8_t *buf, uint32_t len);
