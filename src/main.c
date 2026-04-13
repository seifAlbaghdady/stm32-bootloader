#include "flash.h"
#include "uart.h"
#include "crc32.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SOF 0xA5U
#define ACK 0x06U
#define NAK 0x15U
#define MAX_IMAGE_SIZE (128U * 1024U)
#define RECV_TIMEOUT_MS 5000U
#define APP_SECTOR_START 3
#define APP_SECTOR_END 7

#define SCB_VTOR (*(volatile uint32_t *)0xE000ED08UL)

typedef void (*FuncPtr)(void);
typedef struct
{
    uint32_t *stack_ptr;
    FuncPtr reset_handler;
} VectorTable;

static uint8_t rx_buf[MAX_IMAGE_SIZE];

static void respond(uint8_t code)
{
    uart_send_byte(code);
}

static bool recv_bytes(uint8_t *dst, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        if (!uart_recv_timeout(&dst[i], RECV_TIMEOUT_MS))
            return false;
    }
    return true;
}

static FlashStatus erase_app_area(void)
{
    for (uint8_t s = APP_SECTOR_START; s <= APP_SECTOR_END; s++)
    {
        FlashStatus st = flash_erase_sector(s);
        if (st != FLASH_OK)
            return st;
    }
    return FLASH_OK;
}

static bool receive_and_flash(void)
{
    uint8_t sof;
    if (!uart_recv_timeout(&sof, RECV_TIMEOUT_MS) || sof != SOF)
    {
        uart_send_str("ERR: no SOF\r\n");
        return false;
    }

    uint8_t len_bytes[4];
    if (!recv_bytes(len_bytes, 4))
    {
        uart_send_str("ERR: timeout on length\r\n");
        return false;
    }
    uint32_t img_len;
    memcpy(&img_len, len_bytes, 4);

    if (img_len == 0 || img_len > MAX_IMAGE_SIZE)
    {
        uart_send_str("ERR: invalid length\r\n");
        respond(NAK);
        return false;
    }

    uart_send_str("RDY\r\n");
    if (!recv_bytes(rx_buf, img_len))
    {
        uart_send_str("ERR: timeout on payload\r\n");
        return false;
    }

    uint8_t crc_bytes[4];
    if (!recv_bytes(crc_bytes, 4))
    {
        uart_send_str("ERR: timeout on CRC\r\n");
        return false;
    }
    uint32_t expected_crc;
    memcpy(&expected_crc, crc_bytes, 4);

    uint32_t computed_crc = crc32_compute(rx_buf, img_len);
    if (computed_crc != expected_crc)
    {
        uart_send_str("ERR: CRC mismatch\r\n");
        respond(NAK);
        return false;
    }

    uart_send_str("CRC OK, flashing...\r\n");

    if (flash_unlock() != FLASH_OK)
    {
        uart_send_str("ERR: flash unlock\r\n");
        respond(NAK);
        return false;
    }

    if (erase_app_area() != FLASH_OK)
    {
        flash_lock();
        uart_send_str("ERR: erase failed\r\n");
        respond(NAK);
        return false;
    }

    if (flash_write_buffer(APP_START_ADDR, rx_buf, img_len) != FLASH_OK)
    {
        flash_lock();
        uart_send_str("ERR: write failed\r\n");
        respond(NAK);
        return false;
    }

    flash_lock();

    if (!flash_verify(APP_START_ADDR, rx_buf, img_len))
    {
        uart_send_str("ERR: verify failed\r\n");
        respond(NAK);
        return false;
    }

    flash_unlock();
    flash_erase_sector(2);
    flash_lock();

    respond(ACK);
    uart_send_str("DONE\r\n");
    return true;
}

static void jump_to_app(void)
{

    uint32_t stack_ptr = flash_read_word(APP_START_ADDR);
    if ((stack_ptr & 0x2FF00000UL) != 0x20000000UL)
    {
        uart_send_str("No valid app — staying in bootloader\r\n");
        return;
    }

    uart_send_str("Jumping to app...\r\n");

    SCB_VTOR = APP_START_ADDR;

    const VectorTable *app_vectors = (const VectorTable *)APP_START_ADDR;
    uint32_t app_stack = (uint32_t)app_vectors->stack_ptr;
    FuncPtr app_reset = app_vectors->reset_handler;

    __asm volatile(
        "MSR MSP, %0   \n"
        "BX  %1        \n"
        : : "r"(app_stack), "r"(app_reset) :);
}

int main(void)
{
    uart_init(115200);
    uart_send_str("\r\nSTM32 Bootloader v1.0\r\n");

    uint32_t boot_flag = flash_read_word(BOOT_FLAG_ADDR);

    if (boot_flag == BOOT_FLAG_UPDATE)
    {
        uart_send_str("Update requested — waiting for image...\r\n");
        receive_and_flash();
    }

    jump_to_app();

    for (;;)
    {
    }
}
