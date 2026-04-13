#include "flash.h"

#define FLASH_R_BASE 0x40023C00UL
#define FLASH_ACR (*(volatile uint32_t *)(FLASH_R_BASE + 0x00))
#define FLASH_KEYR (*(volatile uint32_t *)(FLASH_R_BASE + 0x04))
#define FLASH_SR (*(volatile uint32_t *)(FLASH_R_BASE + 0x0C))
#define FLASH_CR (*(volatile uint32_t *)(FLASH_R_BASE + 0x10))

#define FLASH_CR_PG (1UL << 0)
#define FLASH_CR_SER (1UL << 1)
#define FLASH_CR_SNB_Pos 3
#define FLASH_CR_PSIZE_Pos 8
#define FLASH_CR_STRT (1UL << 16)
#define FLASH_CR_LOCK (1UL << 31)

/* FLASH_SR bits */
#define FLASH_SR_BSY (1UL << 16)
#define FLASH_SR_PGSERR (1UL << 7)
#define FLASH_SR_PGPERR (1UL << 6)
#define FLASH_SR_PGAERR (1UL << 5)
#define FLASH_SR_WRPERR (1UL << 4)
#define FLASH_SR_OPERR (1UL << 1)
#define FLASH_SR_EOP (1UL << 0)
#define FLASH_SR_ERRORS (FLASH_SR_PGSERR | FLASH_SR_PGPERR | \
                         FLASH_SR_PGAERR | FLASH_SR_WRPERR | FLASH_SR_OPERR)

#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

#define PSIZE_WORD (2UL << FLASH_CR_PSIZE_Pos)

static FlashStatus wait_for_not_busy(void)
{
    while (FLASH_SR & FLASH_SR_BSY)
    {
    }
    if (FLASH_SR & FLASH_SR_ERRORS)
    {
        FLASH_SR |= FLASH_SR_ERRORS;
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

FlashStatus flash_unlock(void)
{
    if (!(FLASH_CR & FLASH_CR_LOCK))
        return FLASH_OK;
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
    return (FLASH_CR & FLASH_CR_LOCK) ? FLASH_ERROR : FLASH_OK;
}

void flash_lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

FlashStatus flash_erase_sector(uint8_t sector)
{
    FlashStatus s = wait_for_not_busy();
    if (s != FLASH_OK)
        return s;

    FLASH_CR &= ~(0xFUL << FLASH_CR_SNB_Pos);
    FLASH_CR |= FLASH_CR_SER | ((uint32_t)sector << FLASH_CR_SNB_Pos) | PSIZE_WORD;
    FLASH_CR |= FLASH_CR_STRT;

    return wait_for_not_busy();
}

FlashStatus flash_write_word(uint32_t addr, uint32_t data)
{
    FlashStatus s = wait_for_not_busy();
    if (s != FLASH_OK)
        return s;

    FLASH_CR |= FLASH_CR_PG | PSIZE_WORD;
    *(volatile uint32_t *)addr = data;
    s = wait_for_not_busy();
    FLASH_CR &= ~FLASH_CR_PG;

    /* Verify */
    if (*(volatile uint32_t *)addr != data)
        return FLASH_ERROR;
    return s;
}

FlashStatus flash_write_buffer(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t i = 0;
    for (; i + 4 <= len; i += 4)
    {
        uint32_t word;
        __builtin_memcpy(&word, buf + i, 4);
        FlashStatus s = flash_write_word(addr + i, word);
        if (s != FLASH_OK)
            return s;
    }
    if (i < len)
    {
        uint32_t word = 0xFFFFFFFFUL;
        __builtin_memcpy(&word, buf + i, len - i);
        FlashStatus s = flash_write_word(addr + i, word);
        if (s != FLASH_OK)
            return s;
    }
    return FLASH_OK;
}

uint32_t flash_read_word(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

bool flash_verify(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        if (*(volatile uint8_t *)(addr + i) != buf[i])
            return false;
    }
    return true;
}
